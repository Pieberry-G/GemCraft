#include "Core/Scene.h"
#include "Core/ResourceManager.h"

#include "Mesh/GeodesicTool.h"
#include "Mesh/RegionSelectionTool.h"
#include "Mesh/GeometryTool.h"
#include "Mesh/PlacementTool.h"
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
			{ Key::C, GC_BIND_EVENT_FN(Scene::ConstructGeodesicPath)     },
			{ Key::V, GC_BIND_EVENT_FN(Scene::PlaceGemsOnPath)		     },
			{ Key::Q, GC_BIND_EVENT_FN(Scene::RepairSelectedRegion)      },
			{ Key::W, GC_BIND_EVENT_FN(Scene::PlaceGemsOnSelectedRegion) },
			{ Key::P, GC_BIND_EVENT_FN(Scene::PlaceGemsAtTargets)		 },
			{ Key::E, GC_BIND_EVENT_FN(Scene::BooleanOpDifference)		 },
		};

		auto it = functionMap.find(key);
		if (it != functionMap.end()) {
			it->second();
		}
	}

	void Scene::OnRender(const std::string& command)
	{
		static const std::unordered_map<std::string, std::function<void()>> functionMap = {
			{ "ShowRingStroke",		GC_BIND_EVENT_FN(Scene::ShowRingStroke)		},
			{ "ShowSelectedRegion", GC_BIND_EVENT_FN(Scene::ShowSelectedRegion)	},
			{ "ShowSourcePoint",	GC_BIND_EVENT_FN(Scene::ShowSourcePoint)	},
			{ "ShowTargetPoint",	GC_BIND_EVENT_FN(Scene::ShowTargetPoint)	},
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
		m_GeodesicTool = std::make_unique<GeodesicTool>(m_Ring, this);
	}

	void Scene::AddRing(const std::string& name, const std::string& filepath)
	{
		GC_CORE_ASSERT(!m_Ring, "Ring already exists!");

		m_Ring = std::make_shared<Mesh>(name, filepath);
		m_Ring->AddToPolyscope();
		m_Ring->GetPsMesh()->setMaterial("clay");
		m_Ring->GetPsMesh()->setSurfaceColor(glm::vec3(0.750, 0.750, 0.750));
	}

	void Scene::AddMesh(std::shared_ptr<Mesh>& mesh)
	{
		m_Meshes.push_back(mesh);
		m_Meshes.back()->AddToPolyscope();
		m_Meshes.back()->GetPsMesh()->setMaterial("clay");
		m_Meshes.back()->GetPsMesh()->setSurfaceColor(glm::vec3(0.750, 0.750, 0.750));
	}

	void Scene::AutoRecognizeGems()
	{
		RegionSelectionTool regionSelectionTool(this);
		regionSelectionTool.AutoRecognizeGems();
		regionSelectionTool.ShowResult();
	}

	void Scene::AutoSelectRegion()
	{
		RegionSelectionTool regionSelectionTool(this);
		regionSelectionTool.AutoSelectRegion();
		regionSelectionTool.ShowResult();
	}

	void Scene::RepairSelectedRegion()
	{
		GeometryTool geometryTool(this);
		geometryTool.RepairGeometry();
		geometryTool.ShowResult();
	}

	void Scene::ConstructGeodesicPath()
	{
		m_GeodesicPath = m_GeodesicTool->ConstructGeodesicPath();

		// Visualization
		std::vector<std::array<size_t, 2>> edgeInds;
		for (size_t j = 1; j < m_GeodesicPath.Length(); j++) {
			edgeInds.push_back({ j - 1, j });
		}
		polyscope::SurfaceGraphQuantity* test = m_Ring->GetPsMesh()->addSurfaceGraphQuantity("GeodesicPath", m_GeodesicPath.Points(), edgeInds);
		test->setEnabled(true);
		test->setRadius(0.002f);
		test->setColor({ 0.0, 0.0, 0.0 });
	}

	void Scene::PlaceGemsOnPath()
	{
		PlacementTool placerTool(this);
		GemLine gemLine = placerTool.PlaceGemsOnPath(m_GeodesicPath);
		m_GemLines.push_back(gemLine);
	}

	void Scene::PlaceGemsAtTargets()
	{
		PlacementTool placerTool(this);
		GemGroup gemGroup = placerTool.PlaceGemsAtTargets();
		m_GemGroups.push_back(gemGroup);
	}

	void Scene::PlaceGemsOnSelectedRegion()
	{
		PlacementTool placerTool(this);
		if (!m_GemGroups.empty()) {
			for (auto& gemGroup : m_GemGroups) {
				const std::vector<std::shared_ptr<Mesh>>& gems = gemGroup.GetGems();
				for (auto& gem : gems) {
					gem->RemoveFromPolyscope();
				}
				const std::vector<std::shared_ptr<Mesh>>& gemSettings = gemGroup.GetGemSettings();
				for (auto& gemSetting : gemSettings) {
					gemSetting->RemoveFromPolyscope();
				}
			}
		}
		m_GemGroups.clear();
		GemGroup gemGroup = placerTool.PlaceGemsOnSelectedRegion();
		m_GemGroups.push_back(gemGroup);
	}

	void Scene::BooleanOpDifference()
	{
		glm::mat4 transform = m_Ring->GetPsTransform();
		m_Ring->RemoveFromPolyscope();
		BooleanTool booleanTool(this);
		for (auto& gemLine : m_GemLines) {
			m_Ring = booleanTool.DifferenceOperation(m_Ring, gemLine);
		}
		for (auto& gemGroup : m_GemGroups) {
			m_Ring = booleanTool.DifferenceOperation(m_Ring, gemGroup);
		}
		m_Ring->SetName("Ring");
		m_Ring->AddToPolyscope(transform);
	}

	void Scene::ShowRingStroke()
	{
		float strokelinesRadius = 0.002f;

		std::vector<glm::vec3>& positions = polyscope::state::strokePosition;
		std::vector<std::array<size_t, 2>> edgeInds;
		for (size_t i = 1; i < positions.size(); i++) {
			edgeInds.push_back({ i - 1, i });
		}
		polyscope::SurfaceGraphQuantity* strokelines = m_Ring->GetPsMesh()->addSurfaceGraphQuantity("Stroke", positions, edgeInds);
		strokelines->setEnabled(true);
		strokelines->setRadius(strokelinesRadius);
		strokelines->setColor({ 0.0, 0.0, 0.0 });
	}

	void Scene::ShowSelectedRegion()
	{
		std::vector<glm::vec3> faceColors(m_Ring->GetFaces().size());
		for (size_t i = 0; i < m_Ring->GetFaces().size(); i++) {
			faceColors[i] = m_Ring->GetPsMesh()->getSurfaceColor();
		}
		for (std::set<size_t>::iterator it = polyscope::state::selectedRegion.Faces().begin();
			it != polyscope::state::selectedRegion.Faces().end(); ++it) {
			faceColors[*it] = { 0.5, 0, 0 };
		}
		polyscope::SurfaceFaceColorQuantity* showFaces = m_Ring->GetPsMesh()->addFaceColorQuantity("selected region", faceColors);
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
		std::vector<std::array<uint32_t, 2>> vertInd;
		vertPos.push_back(polyscope::state::endPath);

		polyscope::SurfaceGraphQuantity* showVerts = m_Ring->GetPsMesh()->addSurfaceGraphQuantity("target point", vertPos, vertInd);
		showVerts->setEnabled(true);
		showVerts->setRadius(vertexRadius);
		showVerts->setColor({1.0f, 0.0f, 0.0f});
	}

} // namespace GemCraft