#include "Mesh/GeometryTool.h"

#include "Core/Scene.h"
#include "Mesh/FormatTool.h"

#include <polyscope/polyscope.h>
#include <filesystem>

//#include <CGAL/Heat_method_3/Surface_mesh_geodesic_distances_3.h>
//#include <CGAL/Polygon_mesh_processing/distance.h>
//typedef CGALMesh::Property_map<vertex_descriptor, double> Vertex_distance_map;
//typedef CGAL::Heat_method_3::Surface_mesh_geodesic_distances_3<CGALMesh> Heat_method;

namespace GemCraft {

    static bool IsSmallHole(halfedge_descriptor h, CGALMesh& cgalmesh,
        double maxHoleDiam, int maxNumHoleEdges)
    {
        int numHoleEdges = 0;
        CGAL::Bbox_3 holeBBox;
        for (halfedge_descriptor hc : CGAL::halfedges_around_face(h, cgalmesh))
        {
            const CGALPoint& p = cgalmesh.point(target(hc, cgalmesh));
            holeBBox += p.bbox();
            ++numHoleEdges;

            // Exit early, to avoid unnecessary traversal of large holes
            if (numHoleEdges > maxNumHoleEdges) return false;
            if (holeBBox.xmax() - holeBBox.xmin() > maxHoleDiam) return false;
            if (holeBBox.ymax() - holeBBox.ymin() > maxHoleDiam) return false;
            if (holeBBox.zmax() - holeBBox.zmin() > maxHoleDiam) return false;
        }
        return true;
    }

    void GeometryTool::RepairGeometry()
    {
        RemoveSelectedRegion();
        FillHoles();

        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        ring->RemoveFromPolyscope();
        ring = std::make_shared<Mesh>("Ring", m_PatchedMesh->GetVertices(), m_PatchedMesh->GetFaces());
        ring->AddToPolyscope();
        m_Scene->InitGeodesic();
    }

    void GeometryTool::RemoveSelectedRegion()
    {
        GC_CORE_WARN("Removing selected region.");

        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        std::vector<bool> toDelete(ring->GetFaces().size(), false);
        for (auto& index : polyscope::state::selectedRegion.Faces()) {
            toDelete[index] = true;
        }

        std::vector<glm::vec3> vertices = ring->GetVertices();
        std::vector<std::vector<size_t>> faces = ring->GetFaces();
        std::vector<std::vector<size_t>> newFaces;

        for (size_t i = 0; i < faces.size(); i++) {
            if (!toDelete[i]) {
                newFaces.push_back(faces[i]);
            }
        }
        std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>("", vertices, newFaces);
        glm::mat4 transform = newMesh->GetPsTransform();
        glm::mat4 inverseTransform = glm::inverse(transform);
        std::shared_ptr<CGALMesh> cgalmesh = FormatTool::MeshToCGALMesh(newMesh, transform);
        CGALpmp::remove_isolated_vertices(*cgalmesh);
        cgalmesh->collect_garbage();
        m_HollowedMesh = FormatTool::CGALMeshToMesh(cgalmesh, inverseTransform);
        m_HollowedMesh->SetName("3-RemoveSelectedRegion");

        GC_CORE_INFO("Completed!");
    }

    void GeometryTool::FillHoles()
    {
        GC_CORE_WARN("Filling holes in the mesh.");

        glm::mat4 transform = m_HollowedMesh->GetPsTransform();
        glm::mat4 inverseTransform = glm::inverse(transform);
        std::shared_ptr<CGALMesh> cgalmesh = FormatTool::MeshToCGALMesh(m_HollowedMesh, transform);  
        
        // Both of these must be positive in order to be considered
        double maxHoleDiam = -1.0;
        int maxNumHoleEdges = -1;
        
        unsigned int nb_holes = 0;
        std::vector<halfedge_descriptor> borderCycles;
        
        // collect one halfedge per boundary cycle
        CGALpmp::extract_boundary_cycles(*cgalmesh, std::back_inserter(borderCycles));
        
        polyscope::state::selectedRegion.Reset();
        std::vector<typename boost::graph_traits<CGALMesh>::halfedge_descriptor> border_halfedges;
        for (halfedge_descriptor h : borderCycles)
        {
            if (maxHoleDiam > 0 && maxNumHoleEdges > 0 &&
                !IsSmallHole(h, *cgalmesh, maxHoleDiam, maxNumHoleEdges))
                continue;
        
            std::vector<face_descriptor>  patchFaces;
            std::vector<vertex_descriptor> patchVertices;
            bool success = std::get<0>(CGALpmp::triangulate_refine_and_fair_hole(*cgalmesh,
                h,
                CGAL::parameters::face_output_iterator(std::back_inserter(patchFaces))
                .vertex_output_iterator(std::back_inserter(patchVertices))));

            //CGALpmp::triangulate_and_refine_hole(*cgalmesh,
            //    h,
            //    CGAL::parameters::face_output_iterator(std::back_inserter(patchFaces))
            //    .vertex_output_iterator(std::back_inserter(patchVertices)));

            CGAL::Polygon_mesh_processing::border_halfedges(patchFaces, *cgalmesh,
                std::back_inserter(border_halfedges));
        
            GC_CORE_TRACE("* Number of facets in constructed patch: {0}", patchFaces.size());
            GC_CORE_TRACE("* Number of vertices in constructed patch: {0}", patchVertices.size());
            //GC_CORE_TRACE("* Is fairing successful: {0}", success);
            ++nb_holes;

            for (auto& face : patchFaces) {
                polyscope::state::selectedRegion.AddFace(face);
            }
        }
        GC_CORE_TRACE("* {0} holes have been filled.", nb_holes);

        m_PatchedMesh = FormatTool::CGALMeshToMesh(cgalmesh, inverseTransform);
        m_PatchedMesh->SetName("4-FillHole");

        GC_CORE_INFO("Completed!");


        ////property map for the distance values to the source set
        //Vertex_distance_map vertex_distance = cgalmesh->add_property_map<vertex_descriptor, double>("v:distance", 0).first;
        //Heat_method hm(*cgalmesh);
        //for (halfedge_descriptor hed : border_halfedges) {
        //    vertex_descriptor source1 = source(hed, *cgalmesh);
        //    hm.add_source(source1);
        //}
        //hm.estimate_geodesic_distances(vertex_distance);
        //for (vertex_descriptor vd : vertices(*cgalmesh)) {
        //    m_Distance.push_back(get(vertex_distance, vd));
        //}
    }

    void GeometryTool::ShapeSmoothing()
    {
        glm::mat4 transform = m_PatchedMesh->GetPsTransform();
        glm::mat4 inverseTransform = glm::inverse(transform);
        std::shared_ptr<CGALMesh> cgalmesh = FormatTool::MeshToCGALMesh(m_PatchedMesh, transform);

        GC_CORE_WARN("Implementing shape smoothing.");
        std::set<CGALMesh::Vertex_index> constrainedVertices;
        for (size_t faceID = 0; faceID < m_PatchedMesh->nFaces(); faceID++) {
            if (polyscope::state::selectedRegion.Faces().find(faceID) == polyscope::state::selectedRegion.Faces().end()) {
                for (size_t index : m_PatchedMesh->GetFaces()[faceID]) {
                    constrainedVertices.insert(CGALMesh::Vertex_index(index));
                }
            }
        }
        std::cout << "Constraining: " << constrainedVertices.size() << " border vertices" << std::endl;
        CGAL::Boolean_property_map<std::set<CGALMesh::Vertex_index> > vcmap(constrainedVertices);

        CGALpmp::smooth_shape(*cgalmesh, 0.1, CGAL::parameters::number_of_iterations(10)
            .vertex_is_constrained_map(vcmap));

        //CGALpmp::smooth_shape(*cgalmesh, 0.001, CGAL::parameters::number_of_iterations(10));

        m_SmoothedMesh = FormatTool::CGALMeshToMesh(cgalmesh, inverseTransform);
        m_SmoothedMesh->SetName("5-ShapeSmoothing");

        GC_CORE_INFO("Shape smoothing completed!");
    }

    void GeometryTool::ShowResult()
    {
        // 3-RemoveSelectedRegion
        {
            m_Scene->AddMesh(m_HollowedMesh);
            m_HollowedMesh->GetPsMesh()->setEnabled(false);
        }

        // 4-FillHole
        {
            m_Scene->AddMesh(m_PatchedMesh);
            m_PatchedMesh->GetPsMesh()->setEnabled(false);
        }

        // 2-SelectRegion
        {
            std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
            std::vector<glm::vec3> faceColors(ring->GetFaces().size());
            for (size_t i = 0; i < ring->GetFaces().size(); i++) {
                faceColors[i] = ring->GetPsMesh()->getSurfaceColor();
            }
            for (std::set<size_t>::iterator it = polyscope::state::selectedRegion.Faces().begin();
                it != polyscope::state::selectedRegion.Faces().end(); ++it) {
                faceColors[*it] = { 0.5, 0, 0 };
            }
            polyscope::SurfaceFaceColorQuantity* showFaces = ring->GetPsMesh()->addFaceColorQuantity("Selected Region", faceColors);
            showFaces->setEnabled(true);
        }

        //// Geodesic Distance
        //{
        //    auto maxIter = std::max_element(m_Distance.begin(), m_Distance.end());
        //    float maxValue = *maxIter;
        //    std::cout << maxValue << std::endl;
        //
        //    std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        //    std::vector<glm::vec3> vertexColors(ring->GetVertices().size());
        //    for (size_t i = 0; i < ring->GetVertices().size(); i++) {
        //        float value = m_Distance[i] / maxValue;
        //        vertexColors[i] = { value, value, value };
        //    }
        //    polyscope::SurfaceVertexColorQuantity* showFaces = ring->GetPsMesh()->addVertexColorQuantity("Geodesic Distance", vertexColors);
        //    showFaces->setEnabled(true);
        //}

        //// Isolines
        //{
        //    std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        //    std::vector<glm::vec3> positions;
        //    std::vector<std::array<size_t, 2>> edgeInds;
        //    double distBetweenLines = 2.0; // enforce spacing
        //    for (auto& face : ring->GetFaces()) {
        //        std::vector<glm::vec3> pos;
        //        for (size_t i = 0; i < face.size(); i++) {
        //            double vs = m_Distance[face[i]];
        //            double vd = m_Distance[face[(i + 1) % face.size()]];
        //            int region1 = floor(vs / distBetweenLines);
        //            int region2 = floor(vd / distBetweenLines);
        //            if (region1 != region2) {
        //                double val = region2 * distBetweenLines;
        //                if (region1 > region2) {
        //                    val = region1 * distBetweenLines;
        //                }
        //                float t = (val - vs) / (vd - vs);
        //                glm::vec3 ps = ring->GetVertices()[face[i]];
        //                glm::vec3 pd = ring->GetVertices()[face[(i + 1) % face.size()]];
        //                glm::vec3 p = ps + t * (pd - ps);
        //                pos.push_back(p);
        //            }
        //        }
        //        if (pos.size() == 2) {
        //            positions.push_back(pos[0]);
        //            positions.push_back(pos[1]);
        //            edgeInds.push_back({ positions.size() - 2, positions.size() - 1 });
        //        }
        //    }
        //    polyscope::SurfaceGraphQuantity* isolines = ring->GetPsMesh()->addSurfaceGraphQuantity("Isolines", positions, edgeInds);
        //    isolines->setEnabled(true);
        //    isolines->setRadius(0.002f);
        //    isolines->setColor({ 0.0, 0.0, 0.0 });
        //}
    }

} // namespace GemCraft