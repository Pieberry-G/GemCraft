#include "Mesh/GeometryTool.h"

#include "Core/Scene.h"
#include "Mesh/FormatTool.h"

#include <polyscope/polyscope.h>

namespace GemCraft {

    static bool IsSmallHole(halfedge_descriptor h, std::shared_ptr<CGALMesh>& cgalmesh, double maxHoleDiam, int maxNumHoleEdges);
    static std::set<CGAL::SM_Face_index> FindNRingFaces(std::shared_ptr<CGALMesh>& cgalmesh, CGAL::SM_Face_index queryFace, uint32_t nRing);

    void GeometryTool::Clean()
    {
        m_HollowedMesh = nullptr;
        m_PatchedMesh = nullptr;
        m_Distance.clear();
    }

    void GeometryTool::RepairGeometry()
    {
        RemoveSelectedRegion();
        FillHoles();

        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        ring->RemoveFromPolyscope();
        ring = std::make_shared<Mesh>("Ring", m_PatchedMesh->GetVertices(), m_PatchedMesh->GetFaces());
        ring->AddToPolyscope();
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
        std::shared_ptr<CGALMesh> newcgalmesh = FormatTool::MeshToCGALMesh(newMesh, transform);
        CGALpmp::remove_isolated_vertices(*newcgalmesh);
        newcgalmesh->collect_garbage();
        m_HollowedMesh = FormatTool::CGALMeshToMesh(newcgalmesh, inverseTransform);
        m_HollowedMesh->SetName("RemoveSelectedRegion");

        GC_CORE_INFO("Completed!");
    }

    void GeometryTool::FillHoles()
    {
        GC_CORE_WARN("Filling holes in the mesh.");

        const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;

        glm::mat4 transform = m_HollowedMesh->GetPsTransform();
        glm::mat4 inverseTransform = glm::inverse(transform);
        std::shared_ptr<CGALMesh> cgalmesh = FormatTool::MeshToCGALMesh(m_HollowedMesh, transform);  
        
        // Both of these must be positive in order to be considered
        double maxHoleDiam = -1.0;
        int maxNumHoleEdges = -1;
        
        unsigned int nbHoles = 0;
        std::vector<halfedge_descriptor> borderCycles;
        
        // collect one halfedge per boundary cycle
        CGALpmp::extract_boundary_cycles(*cgalmesh, std::back_inserter(borderCycles));
        
        polyscope::state::selectedRegion.Reset();
        for (halfedge_descriptor h : borderCycles) {
            if (maxHoleDiam > 0 && maxNumHoleEdges > 0 &&
                !IsSmallHole(h, cgalmesh, maxHoleDiam, maxNumHoleEdges))
                continue;
        
            std::vector<face_descriptor>  patchFaces;
            std::vector<vertex_descriptor> patchVertices;
            bool success = std::get<0>(CGALpmp::triangulate_refine_and_fair_hole(*cgalmesh,
                h,
                CGAL::parameters::face_output_iterator(std::back_inserter(patchFaces))
                .vertex_output_iterator(std::back_inserter(patchVertices))
                .fairing_continuity(gemPatternUI.GetFairingContinuity())));
            for (auto& face : patchFaces) {
                polyscope::state::selectedRegion.AddFace(face);
            }
            ++nbHoles;

            std::set<CGALMesh::Vertex_index> targetVertices;
            std::set<CGALMesh::Vertex_index> constrainedVertices;
            for (CGALMesh::Face_index f : patchFaces) {
                std::set<CGAL::SM_Face_index> faces = FindNRingFaces(cgalmesh, f, 10);
                for (CGAL::SM_Face_index index : faces) {
                    CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
                    for (boost::tie(vbegin, vend) = cgalmesh->vertices_around_face(cgalmesh->halfedge(index)); vbegin != vend; ++vbegin) {
                        targetVertices.insert(*vbegin);
                    }
                }
            }
            for (auto v : cgalmesh->vertices()) {
                if (targetVertices.find(v) == targetVertices.end()) {
                    constrainedVertices.insert(v);
                }
            }

            CGAL::Boolean_property_map<std::set<CGALMesh::Vertex_index>> vcmap(constrainedVertices);
            CGALpmp::smooth_shape(*cgalmesh, 0.001, CGAL::parameters::number_of_iterations(10)
                .vertex_is_constrained_map(vcmap));
        }
        GC_CORE_TRACE("{0} holes have been filled.", nbHoles);

        CGALpmp::smooth_shape(*cgalmesh, 0.0002, CGAL::parameters::number_of_iterations(10));

        m_PatchedMesh = FormatTool::CGALMeshToMesh(cgalmesh, inverseTransform);
        m_PatchedMesh->SetName("FillHole");

        GC_CORE_INFO("Completed!");
    }

    void GeometryTool::ShowResult()
    {
        // Remove selected region
        {
            m_Scene->AddMesh(m_HollowedMesh);
            m_HollowedMesh->GetPsMesh()->setEnabled(false);
        }

        // Fill hole
        {
            m_Scene->AddMesh(m_PatchedMesh);
            m_PatchedMesh->GetPsMesh()->setEnabled(false);
        }

        // Selected region
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
    }

    static bool IsSmallHole(halfedge_descriptor h, std::shared_ptr<CGALMesh>& cgalmesh, double maxHoleDiam, int maxNumHoleEdges)
    {
        int numHoleEdges = 0;
        CGAL::Bbox_3 holeBBox;
        for (halfedge_descriptor hc : CGAL::halfedges_around_face(h, *cgalmesh)) {
            const CGALPoint& p = cgalmesh->point(target(hc, *cgalmesh));
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

    static std::set<CGAL::SM_Face_index> FindNRingFaces(std::shared_ptr<CGALMesh>& cgalmesh, CGAL::SM_Face_index queryFace, uint32_t nRing)
    {
        Neighbor_query neighborQuery(*cgalmesh);
        std::set<CGAL::SM_Face_index> result;
        std::vector<typename Neighbor_query::Item> neighbors;
        std::queue<std::pair<CGAL::SM_Face_index, size_t>> faceQueue;
        std::unordered_set<CGAL::SM_Face_index> visited;
        faceQueue.push({ queryFace, 0 });
        visited.insert(queryFace);

        while (!faceQueue.empty()) {
            auto [currentFace, distance] = faceQueue.front();
            result.insert(currentFace);
            if (distance < nRing) {
                neighborQuery(currentFace, neighbors);
                for (const auto& neighbor : neighbors) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        faceQueue.push({ neighbor, distance + 1 });
                    }
                }
            }
            faceQueue.pop();
        }
        return result;
    }

} // namespace GemCraft