#include "Mesh/RegionSelectionTool.h"

#include "Core/Scene.h"
#include "Mesh/FormatTool.h"
#include "TinyRenderer/RenderTool.h"

#include <polyscope/polyscope.h>
#include <filesystem>

#include "Mesh/GeodesicTool.h"

#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>

namespace GemCraft {

    struct HalfedgeToEdge
    {
        HalfedgeToEdge(const CGALMesh& cgalmesh, std::vector<edge_descriptor>& edges)
            : m_CGALmesh(cgalmesh), m_Edges(edges) {}

        void operator()(const halfedge_descriptor& h) const
        {
            m_Edges.push_back(edge(h, m_CGALmesh));
        }

        const CGALMesh& m_CGALmesh;
        std::vector<edge_descriptor>& m_Edges;
    };

    static const std::vector<std::array<float, 2>> s_CameraAngles = {
        { 0.0f, 0.0f },
        { 0.0f, 90.0f },
        { 0.0f, 180.0f },
        { 0.0f, 270.0f },
        { 90.0f, 0.0f },
        { -90.0f, 0.0f },
    };

    size_t FindNearestRegion(Neighbor_query& neighborQuery, const Region_growing::Region_map& map, CGAL::SM_Face_index queryFace);
    std::set<size_t> FindNRingFaces(CGALMesh& cgalmesh, CGAL::SM_Face_index queryFace, uint32_t nRing);

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

    void RegionSelectionTool::AutoRecognizeGems()
    {
        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        std::shared_ptr<CGALMesh> cgalmesh = FormatTool::MeshToCGALMesh(ring, ring->GetPsTransform());

        typedef CGALMesh::Property_map<face_descriptor, double> Facet_double_map;
        Facet_double_map sdf_property_map;

        sdf_property_map = cgalmesh->add_property_map<face_descriptor, double>("f:sdf").first;

        // compute SDF values
        // We can't use default parameters for number of rays, and cone angle
        // and the postprocessing
        CGAL::sdf_values(*cgalmesh, sdf_property_map, 2.0 / 3.0 * CGAL_PI, 25, true);

        std::vector<double> sdfValues;
        for (face_descriptor fd : faces(*cgalmesh)) {
            //// ids are between [0, number_of_segments -1]
            //std::cout << sdf_property_map[fd] << " ";
            sdfValues.push_back(sdf_property_map[fd]);
        }
        ring->GetPsMesh()->addFaceScalarQuantity("sdfValues", sdfValues);



        std::vector<bool> toDelete(ring->GetFaces().size(), false);
        for (face_descriptor fd : faces(*cgalmesh)) {
            if (sdf_property_map[fd] < 0.33) {
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
        std::shared_ptr<CGALMesh> hollowedCGALMesh = FormatTool::MeshToCGALMesh(newMesh, transform);
        //CGALpmp::remove_isolated_vertices(*cgalmesh);
        hollowedCGALMesh->collect_garbage();
        auto hollowedMesh = FormatTool::CGALMeshToMesh(hollowedCGALMesh, inverseTransform);
        hollowedMesh->SetName("3-RemoveSelectedRegion");
        hollowedMesh->AddToPolyscope();


        glm::mat4 transform1 = hollowedMesh->GetPsTransform();
        glm::mat4 inverseTransform1 = glm::inverse(transform);
        std::shared_ptr<CGALMesh> patchedCGALMesh = FormatTool::MeshToCGALMesh(hollowedMesh, transform1);

        // Both of these must be positive in order to be considered
        double maxHoleDiam = -1.0;
        int maxNumHoleEdges = -1;

        unsigned int nb_holes = 0;
        std::vector<halfedge_descriptor> borderCycles;

        // collect one halfedge per boundary cycle
        CGALpmp::extract_boundary_cycles(*patchedCGALMesh, std::back_inserter(borderCycles));

        polyscope::state::selectedRegion.Reset();
        std::vector<typename boost::graph_traits<CGALMesh>::halfedge_descriptor> border_halfedges;
        for (halfedge_descriptor h : borderCycles)
        {
            if (maxHoleDiam > 0 && maxNumHoleEdges > 0 &&
                !IsSmallHole(h, *patchedCGALMesh, maxHoleDiam, maxNumHoleEdges))
                continue;

            std::vector<face_descriptor>  patchFaces;
            std::vector<vertex_descriptor> patchVertices;
            bool success = std::get<0>(CGALpmp::triangulate_refine_and_fair_hole(*patchedCGALMesh,
                h,
                CGAL::parameters::face_output_iterator(std::back_inserter(patchFaces))
                .vertex_output_iterator(std::back_inserter(patchVertices))));

            //CGALpmp::triangulate_and_refine_hole(*patchedCGALMesh,
            //    h,
            //    CGAL::parameters::face_output_iterator(std::back_inserter(patchFaces))
            //    .vertex_output_iterator(std::back_inserter(patchVertices)));

            CGAL::Polygon_mesh_processing::border_halfedges(patchFaces, *patchedCGALMesh,
                std::back_inserter(border_halfedges));

            GC_CORE_TRACE("* Number of facets in constructed patch: {0}", patchFaces.size());
            GC_CORE_TRACE("* Number of vertices in constructed patch: {0}", patchVertices.size());
            //GC_CORE_TRACE("* Is fairing successful: {0}", success);
            ++nb_holes;
        }

        auto patchedMesh = FormatTool::CGALMeshToMesh(patchedCGALMesh, inverseTransform1);
        patchedMesh->SetName("4-FillHole");
        patchedMesh->AddToPolyscope();

        RenderMultiviewImages();
        SegmentMultiviewImages();
        ApplyBackProjection();
        RecognizeGems();
    }

    void RegionSelectionTool::AutoSelectRegion()
    {
        RenderMultiviewImages();
        SegmentMultiviewImages();
        ApplyBackProjection();
        SelectRegion();
    }

    void RegionSelectionTool::RenderMultiviewImages()
    {
        GC_CORE_WARN("Rendering multiview images.");

        std::filesystem::path renderOutpath = "../dataIO/InputImages";
        std::filesystem::remove_all(renderOutpath);
        std::filesystem::create_directories(renderOutpath);

        const std::string filepath = m_Scene->GetRing()->GetFilepath();
        TinyRenderer::Model model(filepath);
        TinyRenderer::Camera camera(model.GetRadius() * 2.5f);
        for (size_t i = 0; i < s_CameraAngles.size(); i++) {
            camera.SetEulerAngles(s_CameraAngles[i][0], s_CameraAngles[i][1]);
            TinyRenderer::RenderTool::Render(model, camera);
            TinyRenderer::RenderTool::SaveRenderResult((renderOutpath / std::filesystem::path(filepath).stem()).string() + "_" + std::to_string(i) + ".jpg", 0);
        }

        GC_CORE_INFO("Completed!");
    }

	void RegionSelectionTool::SegmentMultiviewImages()
	{
        GC_CORE_WARN("Implementing segmentation prediction with SAM-Adapter.");

		std::filesystem::create_directories("../dataIO/InputImages");
		std::filesystem::remove_all("../dataIO/OutputMasks");
		std::filesystem::create_directories("../dataIO/OutputMasks");
		system("..\\deps\\sam-adapter\\SegmentInfer.bat");

		GC_CORE_INFO("Completed!");
	}

    void RegionSelectionTool::ApplyBackProjection()
    {
        GC_CORE_WARN("Implementing inverse projection.");

        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        TinyRenderer::Model model(ring->GetVertices(), ring->GetFaces());
        TinyRenderer::Camera camera(model.GetRadius() * 2.5f);

        for (size_t i = 0; i < s_CameraAngles.size(); i++) {
            camera.SetEulerAngles(s_CameraAngles[i][0], s_CameraAngles[i][1]);
            TinyRenderer::RenderTool::Render(model, camera);
            TinyRenderer::RenderTool::BackProjection(m_BackProjectionFaces, "../dataIO/OutputMasks/" + std::to_string(i) + ".png");
        }

        GC_CORE_INFO("Completed!");
    }

    void RegionSelectionTool::RecognizeGems()
    {
        //////////////////////////////////
        // Stage 1: 
        // Region pre-segmentation
        //////////////////////////////////
        GC_CORE_WARN("Implementing region pre-segmentation.");

        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        std::shared_ptr<CGALMesh> cgalmesh = FormatTool::MeshToCGALMesh(ring, ring->GetPsTransform());

        // Default parameter values for the data file building.off.
        const FT        maxDistance = FT(1);
        const FT        maxAngle = FT(15);
        const size_t    minRegionSize = 30;

        // Create instances of the classes Neighbor_query and Region_type.
        Neighbor_query neighborQuery(*cgalmesh);

        Region_type regionType(
            *cgalmesh,
            CGAL::parameters::
            maximum_distance(maxDistance).
            maximum_angle(maxAngle).
            minimum_region_size(minRegionSize));

        // Sort face indices.
        Sorting sorting(*cgalmesh, neighborQuery);
        sorting.sort();

        // Create an instance of the region growing class.
        Region_growing regionGrowing(faces(*cgalmesh), sorting.ordered(), neighborQuery, regionType);

        // Run the algorithm.
        std::vector<typename Region_growing::Primitive_and_region> regions, origions;
        regionGrowing.detect(std::back_inserter(regions));

        {
            // Save regions to a file.
            const std::string fullpath = std::filesystem::path("regions1").stem().string() + ".ply";
            utils::save_polygon_mesh_regions(*cgalmesh, regions, fullpath);
        }

        // Find the nearest region for unassigned faces
        const Region_growing::Region_map& map = regionGrowing.region_map();
        std::vector<typename Region_growing::Item> unassigned;
        regionGrowing.unassigned_items(faces(*cgalmesh), std::back_inserter(unassigned));
        for (auto& unassignedFace : unassigned) {
            size_t nearestRegionIndex = FindNearestRegion(neighborQuery, map, unassignedFace);
            if (nearestRegionIndex != -1) {
                put(map, unassignedFace, nearestRegionIndex);
                regions[nearestRegionIndex].second.push_back(unassignedFace);
            }
        }

        GC_CORE_INFO("Completed!");

        // Save regions to a file.
        const std::string fullpath = std::filesystem::path("regions").stem().string() + ".ply";
        utils::save_polygon_mesh_regions(*cgalmesh, regions, fullpath);


        //////////////////////////////////
        // Stage 2: 
        // Region merging
        //////////////////////////////////
        GC_CORE_WARN("Implementing region merging.");

        // Initialize selected regions
        std::set<size_t> selectedRegions;
        polyscope::state::selectedRegion.Reset();
        polyscope::state::targetPositions.clear();
        polyscope::state::targetNormals.clear();

        std::vector<size_t> nSelectedFaces(regions.size(), 0);
        for (auto& face : m_BackProjectionFaces.Faces()) {
            auto item = CGAL::SM_Face_index(face);
            size_t i = get(map, item);
            nSelectedFaces[i]++;
        }

        for (auto& face : m_BackProjectionFaces.Faces()) {
            auto item = CGAL::SM_Face_index(face);
            size_t i = get(map, item);
            if (selectedRegions.find(i) == selectedRegions.end() && (float)nSelectedFaces[i] / (float)regions[i].second.size() > 0.95f) {
                double area = 0.0;
                for (auto& face : regions[i].second) {
                    area += CGAL::Polygon_mesh_processing::face_area(face, *cgalmesh);
                }
                std::vector<typename boost::graph_traits<CGALMesh>::halfedge_descriptor> border_halfedges;
                CGAL::Polygon_mesh_processing::border_halfedges(regions[i].second, *cgalmesh,
                    std::back_inserter(border_halfedges));
                double perimeter = 0.0;
                for (const auto& h : border_halfedges) {
                    perimeter += CGAL::Polygon_mesh_processing::face_border_length(h, *cgalmesh);
                }
                double shapeRatio = area / (perimeter * perimeter);
            
                if (shapeRatio > 0.006) {
                    std::cout << area << " " << perimeter << std::endl;
                    selectedRegions.insert(i);
                    for (auto& face : regions[i].second) {
                        polyscope::state::selectedRegion.AddFace(face);
                    }
                    
                    std::vector<Point> points;
                    for (auto& face : regions[i].second) {
                        for (auto v : cgalmesh->vertices_around_face(cgalmesh->halfedge(face))) {
                            points.push_back({ cgalmesh->point(v).x(), cgalmesh->point(v).y(), cgalmesh->point(v).z() });
                        }
                    }
                    MinSphere min_sphere(points.begin(), points.end());
                    MinSphere::Cartesian_const_iterator ccib = min_sphere.center_cartesian_begin(), ccie = min_sphere.center_cartesian_end();
                    glm::vec3 center;
                    std::cout << "center:";
                    for (int j = 0; ccib != ccie; ++ccib, ++j) {
                        center[j] = *ccib;
                        std::cout << " " << *ccib;
                    }
                    double radius = min_sphere.radius();
                    std::cout << std::endl << "radius: " << min_sphere.radius() << std::endl;

                    glm::vec3 meanNormal(0, 0, 0);
                    for (auto& face : regions[i].second) {
                        CGAL::Vector_3 normal = CGALpmp::compute_face_normal(face, *cgalmesh);
                        meanNormal += glm::vec3{ normal.x(), normal.y(), normal.z() };
                    }
                    meanNormal /= regions[i].second.size();
                    meanNormal = glm::normalize(meanNormal);
                    std::cout << "mean normal:" << meanNormal.x << " " << meanNormal.y << " " << meanNormal.z << std::endl;

                    polyscope::state::targetPositions.push_back(center);
                    polyscope::state::targetNormals.push_back(meanNormal);
                }
            }
        }

        GC_CORE_INFO("Completed!");
    }

    void RegionSelectionTool::SelectRegion()
    {
        //////////////////////////////////
        // Stage 1: 
        // Region pre-segmentation
        //////////////////////////////////
        GC_CORE_WARN("Implementing region pre-segmentation.");

        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        std::shared_ptr<CGALMesh> cgalmesh = FormatTool::MeshToCGALMesh(ring, ring->GetPsTransform());

        // Default parameter values for the data file building.off.
        const FT        maxDistance = FT(1);
        const FT        maxAngle = FT(15);
        const size_t    minRegionSize = 30;

        // Create instances of the classes Neighbor_query and Region_type.
        Neighbor_query neighborQuery(*cgalmesh);

        Region_type regionType(
            *cgalmesh,
            CGAL::parameters::
            maximum_distance(maxDistance).
            maximum_angle(maxAngle).
            minimum_region_size(minRegionSize));

        // Sort face indices.
        Sorting sorting(*cgalmesh, neighborQuery);
        sorting.sort();

        // Create an instance of the region growing class.
        Region_growing regionGrowing(faces(*cgalmesh), sorting.ordered(), neighborQuery, regionType);

        // Run the algorithm.
        std::vector<typename Region_growing::Primitive_and_region> regions;
        regionGrowing.detect(std::back_inserter(regions));

        // Find the nearest region for unassigned faces
        const Region_growing::Region_map& map = regionGrowing.region_map();
        std::vector<typename Region_growing::Item> unassigned;
        regionGrowing.unassigned_items(faces(*cgalmesh), std::back_inserter(unassigned));
        for (auto& unassignedFace : unassigned) {
            size_t nearestRegionIndex = FindNearestRegion(neighborQuery, map, unassignedFace);
            if (nearestRegionIndex != -1) {
                put(map, unassignedFace, nearestRegionIndex);
                regions[nearestRegionIndex].second.push_back(unassignedFace);
            }
        }

        GC_CORE_INFO("Completed!");

        // Save regions to a file.
        const std::string fullpath = std::filesystem::path("regions").stem().string() + ".ply";
        utils::save_polygon_mesh_regions(*cgalmesh, regions, fullpath);


        //////////////////////////////////
        // Stage 2: 
        // Region merging
        //////////////////////////////////
        GC_CORE_WARN("Implementing region merging.");

        // Initialize selected regions
        std::set<size_t> selectedRegions;
        polyscope::state::selectedRegion.Reset();

        for (auto& face : m_BackProjectionFaces.Faces()) {
            auto item = CGAL::SM_Face_index(face);
            size_t i = get(map, item);
            if (selectedRegions.find(i) == selectedRegions.end()) {
                selectedRegions.insert(i);
                for (auto& face : regions[i].second) {
                    polyscope::state::selectedRegion.AddFace(face);
                }
            }
        }

        // Iterative region merging 
        for (size_t iter = 0; iter < 6; iter++) {
            MeshSubset tintedFaces;
            std::vector<size_t> nTintedFaces(regions.size(), 0);
            for (auto& i : selectedRegions) {
                for (auto& face : regions[i].second) {
                    auto item = CGAL::SM_Face_index(face);
                    std::set<size_t> result = FindNRingFaces(*cgalmesh, item, 5);
                    for (auto& j : result) {
                        tintedFaces.AddFace(j);
                    }
                }
            }
            for (auto& face : tintedFaces.Faces()) {
                nTintedFaces[get(map, CGAL::SM_Face_index(face))]++;
            }
            for (size_t i = 0; i < regions.size(); i++) {
                double area = 0.0;
                for (auto& face : regions[i].second) {
                    area += CGAL::Polygon_mesh_processing::face_area(face, *cgalmesh);
                }
                std::vector<typename boost::graph_traits<CGALMesh>::halfedge_descriptor> border_halfedges;
                CGAL::Polygon_mesh_processing::border_halfedges(regions[i].second, *cgalmesh,
                    std::back_inserter(border_halfedges));
                double perimeter = 0.0;
                for (const auto& h : border_halfedges) {
                    perimeter += CGAL::Polygon_mesh_processing::face_border_length(h, *cgalmesh);
                }
                double shapeRatio = area / (perimeter * perimeter);

                if (selectedRegions.find(i) == selectedRegions.end() && 
                    ((float)nTintedFaces[i] / (float)regions[i].second.size() > 0.5f)
                    || (shapeRatio > 0.002 && (float)nTintedFaces[i] / (float)regions[i].second.size() > 0.3f)
                    ) {
                    selectedRegions.insert(i);
                    for (auto& face : regions[i].second) {
                        polyscope::state::selectedRegion.AddFace(face);
                    }
                }
            }
        }

        GC_CORE_INFO("Completed!");
    }

    size_t FindNearestRegion(Neighbor_query& neighborQuery, const Region_growing::Region_map& map, CGAL::SM_Face_index queryFace)
    {
        std::vector<typename Neighbor_query::Item> neighbors;

        std::queue<CGAL::SM_Face_index> queue;
        std::unordered_set<CGAL::SM_Face_index> visited;
        queue.push(queryFace);
        visited.insert(queryFace);

        while (!queue.empty()) {
            CGAL::SM_Face_index currentFace = queue.front();
            queue.pop();

            neighborQuery(currentFace, neighbors);
            for (const auto& neighbor : neighbors) {
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    queue.push(neighbor);

                    size_t regionIndex = get(map, neighbor);
                    if (regionIndex != size_t(-1)) {
                        return regionIndex;
                    }
                }
            }
        }
        return -1;
    }

    std::set<size_t> FindNRingFaces(CGALMesh& cgalmesh, CGAL::SM_Face_index queryFace, uint32_t nRing)
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

    void RegionSelectionTool::ShowResult()
    {
        // 1-ApplyBackProjection
        {
            std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
            std::vector<glm::vec3> faceColors(ring->GetFaces().size());
            for (size_t i = 0; i < ring->GetFaces().size(); i++) {
                faceColors[i] = ring->GetPsMesh()->getSurfaceColor();
            }
            for (std::set<size_t>::iterator it = m_BackProjectionFaces.Faces().begin();
                it != m_BackProjectionFaces.Faces().end(); ++it) {
                faceColors[*it] = { 0.5, 0, 0.5 };
            }
            polyscope::SurfaceFaceColorQuantity* showFaces = ring->GetPsMesh()->addFaceColorQuantity("Back Projection", faceColors);
            showFaces->setEnabled(false);
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
    }

} // namespace GemCraft