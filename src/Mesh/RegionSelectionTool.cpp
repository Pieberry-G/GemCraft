#include "Mesh/RegionSelectionTool.h"

#include "Core/Scene.h"
#include "Mesh/FormatTool.h"
#include "TinyRenderer/RenderTool.h"
#include "Mesh/GeodesicTool.h"

#include <polyscope/polyscope.h>
#include <filesystem>

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

    static size_t FindNearestRegion(std::shared_ptr<CGALMesh>& cgalmesh, const Region_growing::Region_map& map, CGAL::SM_Face_index queryFace);
    static std::set<CGAL::SM_Face_index> FindNRingFaces(std::shared_ptr<CGALMesh>& cgalmesh, CGAL::SM_Face_index queryFace, uint32_t nRing);

    void RegionSelectionTool::Clean()
    {
        m_BackProjectionFaces.Reset();
    }

    void RegionSelectionTool::AutoRecognizeGems()
    {
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

    void RegionSelectionTool::InteractiveSphereSelect()
    {
        const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;
        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        const std::vector<glm::vec3>& vertices = ring->GetVertices();
        const std::vector<std::vector<size_t>>& faces = ring->GetFaces();
        size_t interactiveFace = polyscope::state::interactiveFace;
        glm::vec3 iFCenter = (vertices[faces[interactiveFace][0]] + vertices[faces[interactiveFace][1]] + vertices[faces[interactiveFace][2]]) / 3.0f;
        for (size_t i = 0; i < faces.size(); i++) {
            glm::vec3 center = (vertices[faces[i][0]] + vertices[faces[i][1]] + vertices[faces[i][2]]) / 3.0f;
            if (glm::length(center - iFCenter) < gemPatternUI.GetSphereToolRadius()) {
                polyscope::state::selectedRegion.AddFace(i);
            }
        }
        ShowResult();
    }

    void RegionSelectionTool::InteractiveFillRegion()
    {
        std::set<size_t> boundaryFaces = polyscope::state::selectedRegion.Faces();
        CGAL::SM_Face_index queryFace = CGAL::SM_Face_index(polyscope::state::interactiveFace);
        if (boundaryFaces.find(queryFace) != boundaryFaces.end()) return;

        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
        std::shared_ptr<CGALMesh> cgalmesh = FormatTool::MeshToCGALMesh(ring, ring->GetPsTransform());
        Neighbor_query neighborQuery(*cgalmesh);
        std::set<CGAL::SM_Face_index> result;
        std::vector<typename Neighbor_query::Item> neighbors;
        std::queue<CGAL::SM_Face_index> faceQueue;
        std::unordered_set<CGAL::SM_Face_index> visited;
        faceQueue.push(queryFace);
        visited.insert(queryFace);
        polyscope::state::selectedRegion.AddFace(queryFace);

        while (!faceQueue.empty()) {
            auto currentFace = faceQueue.front();
            result.insert(currentFace);
            neighborQuery(currentFace, neighbors);
            for (const auto& neighbor : neighbors) {
                if (visited.find(neighbor) == visited.end() && boundaryFaces.find(neighbor) == boundaryFaces.end()) {
                    visited.insert(neighbor);
                    faceQueue.push(neighbor);
                    polyscope::state::selectedRegion.AddFace(neighbor);
                }
            }
            faceQueue.pop();
        }
    }

    void RegionSelectionTool::RenderMultiviewImages()
    {
        GC_CORE_WARN("Rendering multiview images.");

        std::filesystem::path renderOutpath = "../dataIO/InputImages";
        std::filesystem::remove_all(renderOutpath);
        std::filesystem::create_directories(renderOutpath);

        const std::string filepath = m_Scene->GetRingPath();
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
            size_t nearestRegionIndex = FindNearestRegion(cgalmesh, map, unassignedFace);
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
            size_t nearestRegionIndex = FindNearestRegion(cgalmesh, map, unassignedFace);
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
                    std::set<CGAL::SM_Face_index> result = FindNRingFaces(cgalmesh, item, 5);
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

    static size_t FindNearestRegion(std::shared_ptr<CGALMesh>& cgalmesh, const Region_growing::Region_map& map, CGAL::SM_Face_index queryFace)
    {
        Neighbor_query neighborQuery(*cgalmesh);
        std::vector<typename Neighbor_query::Item> neighbors;
        std::queue<CGAL::SM_Face_index> queue;
        std::unordered_set<CGAL::SM_Face_index> visited;
        queue.push(queryFace);
        visited.insert(queryFace);

        while (!queue.empty()) {
            CGAL::SM_Face_index currentFace = queue.front();
            neighborQuery(currentFace, neighbors);
            for (const auto& neighbor : neighbors) {
                if (visited.count(neighbor) == 0) {
                    visited.insert(neighbor);
                    queue.push(neighbor);
                    size_t regionIndex = get(map, neighbor);
                    if (regionIndex != size_t(-1)) {
                        return regionIndex;
                    }
                }
            }
            queue.pop();
        }
        return -1;
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