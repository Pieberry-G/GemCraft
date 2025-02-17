#include "Mesh/SurfaceCleanerTool.h"

#include "Core/Scene.h"
#include "Mesh/FormatTool.h"

#include <omp.h>

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

    static std::set<size_t> FindNRingFaces(CGALMesh& cgalmesh, CGAL::SM_Face_index queryFace, uint32_t nRing);

    void SurfaceCleanerTool::CleanSurface()
    {
        RemoveProngs();
        FillHoles();

        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        m_OriginalMesh = std::make_shared<Mesh>(*ring);
        m_OriginalMesh->SetName("0-OriginalMesh");
        ring->RemoveFromPolyscope();
        ring = std::make_shared<Mesh>("Ring", m_PatchedMesh->GetVertices(), m_PatchedMesh->GetFaces());
        ring->AddToPolyscope();
    }

    void SurfaceCleanerTool::RemoveProngs()
    {
        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        std::shared_ptr<CGALMesh> cgalmesh = FormatTool::MeshToCGALMesh(ring, ring->GetPsTransform());

        typedef CGALMesh::Property_map<face_descriptor, double> Facet_double_map;
        Facet_double_map sdfPropertyMap;

        sdfPropertyMap = cgalmesh->add_property_map<face_descriptor, double>("f:sdf").first;

        // compute SDF values
        // We can't use default parameters for number of rays, and cone angle
        // and the postprocessing
        CGAL::sdf_values(*cgalmesh, sdfPropertyMap, 2.0 / 3.0 * CGAL_PI, 25, true);

        std::cout << "sdf" << std::endl;

        std::vector<double> sdfValues;
        for (face_descriptor fd : faces(*cgalmesh)) {
            //// ids are between [0, number_of_segments -1]
            sdfValues.push_back(sdfPropertyMap[fd]);
        }
        ring->GetPsMesh()->addFaceScalarQuantity("sdfValues", sdfValues);

        std::vector<bool> toDelete(ring->GetFaces().size(), false);
        for (face_descriptor fd : faces(*cgalmesh)) {
            if (sdfPropertyMap[fd] < 0.33) {
                toDelete[fd] = true;
                std::set<size_t> faces = FindNRingFaces(*cgalmesh, fd, 8);
                for (size_t f : faces) {
                    toDelete[f] = true;
                }
            }
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
        std::shared_ptr<CGALMesh> newcgalmesh = FormatTool::MeshToCGALMesh(newMesh, transform);
        //CGALpmp::remove_isolated_vertices(*cgalmesh);
        newcgalmesh->collect_garbage();
        m_HollowedMesh = FormatTool::CGALMeshToMesh(newcgalmesh, inverseTransform);
        m_HollowedMesh->SetName("3-RemoveSelectedRegion");

        std::cout << "remove" << std::endl;
    }

    void SurfaceCleanerTool::FillHoles()
    {
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

            CGAL::Polygon_mesh_processing::border_halfedges(patchFaces, *cgalmesh,
                std::back_inserter(border_halfedges));

            GC_CORE_TRACE("* Number of facets in constructed patch: {0}", patchFaces.size());
            GC_CORE_TRACE("* Number of vertices in constructed patch: {0}", patchVertices.size());
            GC_CORE_TRACE("* Is fairing successful: {0}", success);
            ++nb_holes;
        }

        m_PatchedMesh = FormatTool::CGALMeshToMesh(cgalmesh, inverseTransform);
        m_PatchedMesh->SetName("4-FillHole");

        std::cout << "fillhole" << std::endl;
    }

    static std::set<size_t> FindNRingFaces(CGALMesh& cgalmesh, CGAL::SM_Face_index queryFace, uint32_t nRing)
    {
        Neighbor_query neighborQuery(cgalmesh);

        std::set<size_t> result;
        std::vector<typename Neighbor_query::Item> neighbors;

        std::queue<CGAL::SM_Face_index> faceQueue;
        std::queue<size_t> disQueue;
        std::unordered_set<CGAL::SM_Face_index> visited;
        faceQueue.push(queryFace);
        disQueue.push(0);
        visited.insert(queryFace);

        while (!faceQueue.empty()) {
            CGAL::SM_Face_index currentFace = faceQueue.front();
            faceQueue.pop();
            size_t distance = disQueue.front();
            disQueue.pop();
            result.insert(currentFace);

            if (distance < nRing) {
                neighborQuery(currentFace, neighbors);
                for (const auto& neighbor : neighbors) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        faceQueue.push(neighbor);
                        disQueue.push(distance + 1);
                    }
                }
            }
        }
        return result;
    }

    void SurfaceCleanerTool::ShowResult()
    {
        // 0-OriginalRegion
        {
            m_Scene->AddMesh(m_OriginalMesh);
            m_OriginalMesh->GetPsMesh()->setEnabled(false);
        }

        // 1-RemoveSelectedRegion
        {
            m_Scene->AddMesh(m_HollowedMesh);
            m_HollowedMesh->GetPsMesh()->setEnabled(false);
        }

        // 2-FillHole
        {
            m_Scene->AddMesh(m_PatchedMesh);
            m_PatchedMesh->GetPsMesh()->setEnabled(false);
        }
    }

} // namespace GemCraft