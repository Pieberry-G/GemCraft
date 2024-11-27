#include "Core/Scene.h"
#include "Core/ResourceManager.h"

#include "Mesh/Geodesic.h"
#include "Mesh/PlacerTool.h"
#include "Mesh/BooleanTool.h"

namespace GemCraft {

	Scene::Scene()
	{
		m_GemSelectionUI.Init();
		polyscope::state::userCallbacks.push_back(m_GemSelectionUI.GetDrawUIFunction());

		m_GemSettingSelectionUI.Init();
		polyscope::state::userCallbacks.push_back(m_GemSettingSelectionUI.GetDrawUIFunction());

		polyscope::state::userCallbacks.push_back(m_GemPatternUI.GetDrawUIFunction());
	}

	void Scene::OnKeyReleased(KeyCode key)
	{
		static const std::unordered_map<KeyCode, std::function<void()>> functionMap = {
			//{ Key::W,  GC_BIND_EVENT_FN(Scene::AdornStrokeWithGems)   },
			{ Key::E,  GC_BIND_EVENT_FN(Scene::BooleanOpDifference)   },
			{ Key::C,  GC_BIND_EVENT_FN(Scene::ConstructGeodesicPath) },
			{ Key::V,  GC_BIND_EVENT_FN(Scene::PlaceGemsOnPath)		  },
		};

		auto it = functionMap.find(key);
		if (it != functionMap.end()) {
			it->second();
		}
	}

	void Scene::OnRender(const std::string& command)
	{
		static const std::unordered_map<std::string, std::function<void()>> functionMap = {
			{ "ShowRingStroke",	  GC_BIND_EVENT_FN(Scene::ShowRingStroke)	},
			{ "ShowRingSelected", GC_BIND_EVENT_FN(Scene::ShowRingSelected)	},
			{ "ShowSourcePoint",  GC_BIND_EVENT_FN(Scene::ShowSourcePoint)	},
			{ "ShowTargetPoint",  GC_BIND_EVENT_FN(Scene::ShowTargetPoint)	},
		};

		auto it = functionMap.find(command);
		if (it != functionMap.end()) {
			it->second();
		} else {
			GC_CORE_ERROR("Could nou find the relevant function!");
		}
	}

	void Scene::InitGeodesic()
	{
		m_Geodesic = std::make_unique<Geodesic>(m_Ring);
	}

	void Scene::AddRing(const std::string& name, const std::string& filepath)
	{
		GC_CORE_ASSERT(!m_Ring, "Ring already exists!");

		m_Ring = std::make_shared<Mesh>(name, filepath);
		m_Ring->AddToPolyscope();
		m_Ring->GetPsMesh()->setMaterial("clay");
		//m_Ring->GetPsMesh()->setSurfaceColor(glm::vec3(0.717, 0.498, 0.866));
		m_Ring->GetPsMesh()->setSurfaceColor(glm::vec3(0.750, 0.750, 0.750));
	}

	void Scene::PlaceGemsOnPath()
	{
		PlacerTool placerTool(this);
		for (const Path& path : m_GeodesicPaths) {
			GemLine gemLine = placerTool.PlaceGemsOnPath(path);
			m_GemLines.push_back(gemLine);
		}
	}

	void Scene::BooleanOpDifference()
	{
		glm::mat4 transform = m_Ring->GetPsTransform();
		m_Ring->RemoveFromPolyscope();
		BooleanTool booleanTool(this);
		for (auto& gemLine : m_GemLines) {
			m_Ring = booleanTool.DifferenceOperation(m_Ring, gemLine);
		}
		m_Ring->SetName("Ring");
		m_Ring->AddToPolyscope(transform);
	}

	void Scene::ConstructGeodesicPath()
	{
		int numberOfPaths = m_GemPatternUI.GetNumberOfPaths();
		float pathSpacing = m_GemPatternUI.GetPathSpacing();
		Path geodesicPath = m_Geodesic->ConstructGeodesicPath();
		std::vector<Path> parallelPaths = m_Geodesic->CalculateParallelPaths(geodesicPath, numberOfPaths, pathSpacing);

		for (size_t i = 0; i < m_GeodesicPaths.size(); i++) {
			m_Ring->GetPsMesh()->removeQuantity("GeodesicPath(" + std::to_string(i) + ")");
		}
		m_GeodesicPaths.clear();
		std::copy(parallelPaths.begin(), parallelPaths.end(), std::back_inserter(m_GeodesicPaths));

		// Visualization
		std::vector<std::array<size_t, 2>> edgeInds;
		for (size_t j = 1; j < geodesicPath.Length(); j++) {
			edgeInds.push_back({ j - 1, j });
		}
		for (size_t i = 0; i < m_GeodesicPaths.size(); i++) {
			polyscope::SurfaceGraphQuantity* test = m_Ring->GetPsMesh()->addSurfaceGraphQuantity("GeodesicPath(" + std::to_string(i) + ")", m_GeodesicPaths[i].Points(), edgeInds);
			test->setEnabled(true);
			test->setRadius(0.002f);
			test->setColor({ 0.0, 0.0, 0.0 });
		}
	}

	void Scene::ShowRingStroke()
	{
		float strokelinesRadius = 0.002f;

		std::vector<glm::vec3>& positions = polyscope::state::strokePosition;
		std::vector<std::array<size_t, 2>> edgeInds;
		for (size_t i = 1; i < positions.size(); i++) {
			edgeInds.push_back({ i - 1, i });
		}
		polyscope::SurfaceMesh* ring = m_Ring->GetPsMesh();
		polyscope::SurfaceGraphQuantity* strokelines = ring->addSurfaceGraphQuantity("Stroke", positions, edgeInds);
		strokelines->setEnabled(true);
		strokelines->setRadius(strokelinesRadius);
		strokelines->setColor({ 0.0, 0.0, 0.0 });
	}

	void Scene::ShowRingSelected()
	{
		// Show selected faces.
		std::vector<std::array<double, 3>> faceColors(m_Ring->GetFaces().size());
		for (size_t i = 0; i < m_Ring->GetFaces().size(); i++) {
			faceColors[i] = { 0.0f, 0.0f, 1.0f };
		}
		for (std::set<size_t>::iterator it = polyscope::state::subset.faces.begin();
			it != polyscope::state::subset.faces.end(); ++it) {
			faceColors[*it] = { 0.5, 0, 0.5 };
		}
		for (std::set<size_t>::iterator it = polyscope::state::subset.faces.begin();
		    it != polyscope::state::subset.faces.end(); ++it) {
		    faceColors[*it] = { 0.5, 0, 0.5 };
		}
		//m_Ring->GetPsMesh()->ensureHaveManifoldConnectivity();
		//for (std::set<size_t>::iterator it = polyscope::state::subset.halfedges.begin();
		//	it != polyscope::state::subset.halfedges.end(); ++it) {
		//	faceColors[m_Ring->GetPsMesh()->faceForHalfedge[*it]] = { 0.5, 0, 0.5 };
		//}
		polyscope::SurfaceFaceColorQuantity* showFaces = m_Ring->GetPsMesh()->addFaceColorQuantity("selected faces", faceColors);
		showFaces->setEnabled(true);
	}

	void Scene::ShowSourcePoint()
	{
		float vertexRadius = 0.01f;
		std::vector<glm::vec3> vertPos;
		std::vector<std::array<size_t, 2>> vertInd;
		vertPos.push_back(polyscope::state::startPath);

		polyscope::SurfaceGraphQuantity* showVerts = m_Ring->GetPsMesh()->addSurfaceGraphQuantity("source point", vertPos, vertInd);
		showVerts->setEnabled(true);
		showVerts->setRadius(vertexRadius);
		showVerts->setColor({0.0f, 0.0f, 1.0f});
	}

	void Scene::ShowTargetPoint()
	{
		float vertexRadius = 0.01f;
		std::vector<glm::vec3> vertPos;
		std::vector<std::array<size_t, 2>> vertInd;
		vertPos.push_back(polyscope::state::endPath);

		polyscope::SurfaceGraphQuantity* showVerts = m_Ring->GetPsMesh()->addSurfaceGraphQuantity("target point", vertPos, vertInd);
		showVerts->setEnabled(true);
		showVerts->setRadius(vertexRadius);
		showVerts->setColor({1.0f, 0.0f, 0.0f});
	}

	//void Scene::AdornStrokeWithGems()
	//{
	//	std::string filepath = m_GemSelectionUI.GetCurSelectedGem();

	//	std::string meshname;
	//	std::vector<glm::vec3>& positions = polyscope::state::strokePosition;
	//	std::vector<glm::vec3>& normals = polyscope::state::strokeNormal;

	//	float totalDistance = 0.0f;
	//	for (size_t i = 0; i < positions.size(); i++) {
	//		if (i > 0) {
	//			totalDistance += glm::length(glm::vec3(positions[i] - positions[i - 1]));
	//		}
	//		if (i == 0 || totalDistance > 1.0f) {
	//			meshname = "Gem " + std::to_string(positions[i].x) + " " + std::to_string(positions[i].y) + " " + std::to_string(positions[i].z);
	//			AddGem(meshname, filepath, positions[i], glm::cross(normals[i], glm::vec3(1.0f, 0.0f, 0.0f)), normals[i]);
	//			totalDistance = 0.0f;
	//		}
	//	}
	//}

/*
	static void CalculatePositionsAndForwards(const Path& path, int num, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& forwards)
	{
		const std::vector<glm::vec3>& points = path.Points();
		size_t len = path.Length();
		std::vector<glm::vec3> originForwards;
		originForwards.push_back(glm::normalize(points[1] - points[0]));
		for (size_t i = 1; i < len - 1; i++) {
			originForwards.push_back(glm::normalize(points[i + 1] - points[i - 1]));
		}
		originForwards.push_back(glm::normalize(points[len - 1] - points[len - 2]));

		float pathLength = 0.0f;
		for (size_t i = 1; i < points.size(); i++) {
			pathLength += glm::distance(points[i - 1], points[i]);
		}
		for (int i = 0; i <= (num - 1); i++) {
			float segmentFraction = i / (float)(num - 1);
			float distanceAlongPath = segmentFraction * pathLength;
			size_t currentSegment = 0;
			float currentDistance = 0.0f;
			while (currentSegment + 1 < points.size()) {
				float segmentDistance = glm::distance(points[currentSegment], points[currentSegment + 1]);
				if (currentDistance + segmentDistance >= distanceAlongPath) {
					break;
				}
				currentDistance += segmentDistance;
				++currentSegment;
			}
			float remainingDistance = distanceAlongPath - currentDistance;
			float t = remainingDistance / glm::distance(points[currentSegment], points[currentSegment + 1]);
			glm::vec3 position = glm::mix(points[currentSegment], points[currentSegment + 1], t);
			glm::vec3 forward = glm::mix(originForwards[currentSegment], originForwards[currentSegment + 1], t);
			positions.push_back(position);
			forwards.push_back(forward);
		}
	}
*/

} // namespace GemCraft