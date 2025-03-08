#include "Core/Scene.h"
#include "Core/ResourceManager.h"

#include "Mesh/GeodesicTool.h"
#include "Mesh/SurfaceCleanerTool.h"
#include "Mesh/RegionSelectionTool.h"
#include "Mesh/GeometryTool.h"
#include "Mesh/PlacementTool.h"
#include "Mesh/BooleanTool.h"

namespace GemCraft {

	Scene::Scene()
	{
		m_GemSettingSelectionUI.Init();
		polyscope::state::userCallbacks.push_back(m_GemSettingSelectionUI.GetDrawUIFunction());
		polyscope::state::userCallbacks.push_back(m_GemPatternUI.GetDrawUIFunction());

		m_SurfaceCleanerTool = std::make_unique<SurfaceCleanerTool>(this);
		m_RegionSelectionTool = std::make_unique<RegionSelectionTool>(this);
		m_GeometryTool = std::make_unique<GeometryTool>(this);
		m_PlacementTool = std::make_unique<PlacementTool>(this);
		m_BooleanTool = std::make_unique<BooleanTool>();
	}

	void Scene::Clean()
	{
		polyscope::removeAllStructures();
		polyscope::state::selectedRegion.Reset();
		m_RingPath = "";
		m_Ring = nullptr;
		m_Meshes.clear();
		m_GemLines.clear();
		m_GemGroups.clear();

		m_SurfaceCleanerTool->Clean();
		m_RegionSelectionTool->Clean();
		m_GeometryTool->Clean();
		m_PlacementTool->Clean();
	}

	bool Scene::OnKeyReleased(KeyReleasedEvent& e)
	{
		static const std::unordered_map<KeyCode, std::function<void()>> functionMap = {
			{ Key::Q, GC_BIND_EVENT_FN(Scene::AutoSelectRegion)		     },
			{ Key::W, GC_BIND_EVENT_FN(Scene::RepairSelectedRegion)      },
			{ Key::E, GC_BIND_EVENT_FN(Scene::DigHoleOnSelectedRegion)	 },
			{ Key::R, GC_BIND_EVENT_FN(Scene::PlaceGemsOnSelectedRegion) },
			{ Key::T, GC_BIND_EVENT_FN(Scene::BooleanOpDifference)		 },

			{ Key::Y, GC_BIND_EVENT_FN(Scene::CleanSurface)				 },
			{ Key::U, GC_BIND_EVENT_FN(Scene::AutoRecognizeGems)		 },
			{ Key::I, GC_BIND_EVENT_FN(Scene::PlaceGemsAtTargets)		 },
			{ Key::O, GC_BIND_EVENT_FN(Scene::BooleanOpDifference)		 },
		};

		KeyCode key = e.GetKeyCode();
		auto it = functionMap.find(key);
		if (it != functionMap.end()) {
			it->second();
			return true;
		}
		else {
			return false;
		}
	}

	bool Scene::OnRender(AppRenderEvent& e)
	{
		static const std::unordered_map<std::string, std::function<void()>> functionMap = {
			{ "ShowRingStroke",		     GC_BIND_EVENT_FN(Scene::ShowRingStroke)		  },
			{ "ShowSelectedRegion",      GC_BIND_EVENT_FN(Scene::ShowSelectedRegion)	  },
			{ "InteractiveSphereSelect", GC_BIND_EVENT_FN(Scene::InteractiveSphereSelect) },
			{ "InteractiveFillRegion",   GC_BIND_EVENT_FN(Scene::InteractiveFillRegion)   },
			{ "ShowSourcePoint",	     GC_BIND_EVENT_FN(Scene::ShowSourcePoint)	      },
			{ "ShowTargetPoint",	     GC_BIND_EVENT_FN(Scene::ShowTargetPoint)	      },
		};

		std::string command = e.GetCommand();
		auto it = functionMap.find(command);
		if (it != functionMap.end()) {
			it->second();
			return true;
		} else {
			GC_CORE_ERROR("Could not find the relevant function!");
			return false;
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
		
		m_RingPath = filepath;
	}

	void Scene::AddMesh(std::shared_ptr<Mesh>& mesh)
	{
		m_Meshes.push_back(mesh);
		m_Meshes.back()->AddToPolyscope();
		m_Meshes.back()->GetPsMesh()->setMaterial("clay");
		m_Meshes.back()->GetPsMesh()->setSurfaceColor(glm::vec3(0.750, 0.750, 0.750));
	}

	void Scene::AutoSelectRegion()
	{
		m_RegionSelectionTool->AutoSelectRegion();
		m_RegionSelectionTool->ShowResult();
	}

	void Scene::RepairSelectedRegion()
	{
		m_GeometryTool->RepairGeometry();
		m_GeometryTool->ShowResult();

		InitGeodesic();
		m_PlacementTool->BuildSubmeshForSelectedRegion();
		m_PlacementTool->ParameterizeSubmesh();
		m_PlacementTool->CalculateGeodesicDistance();
	}

	void Scene::DigHoleOnSelectedRegion()
	{
		m_PlacementTool->CreateBooleanMeshForSink();
		m_PlacementTool->ShowResult();

		glm::mat4 transform = m_Ring->GetPsTransform();
		m_Ring->RemoveFromPolyscope();
		m_Ring = m_BooleanTool->DifferenceOperation(m_Ring, m_PlacementTool->GetBooleanMesh());
		m_Ring->SetName("Ring");
		m_Ring->AddToPolyscope(transform);
	}

	void Scene::BooleanOpDifference()
	{
		glm::mat4 transform = m_Ring->GetPsTransform();
		m_Ring->RemoveFromPolyscope();
		for (auto& gemLine : m_GemLines) {
			m_Ring = m_BooleanTool->DifferenceOperation(m_Ring, gemLine);
		}
		for (auto& gemGroup : m_GemGroups) {
			m_Ring = m_BooleanTool->DifferenceOperation(m_Ring, gemGroup);
		}
		m_Ring->SetName("Ring");
		m_Ring->AddToPolyscope(transform);
	}

	void Scene::PlaceGemsOnSelectedRegion()
	{
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
		GemGroup gemGroup = m_PlacementTool->PlaceGemsOnSelectedRegion();
		m_GemGroups.push_back(gemGroup);
	}

	void Scene::CleanSurface()
	{
		m_SurfaceCleanerTool->CleanSurface();
		m_SurfaceCleanerTool->ShowResult();
	}

	void Scene::AutoRecognizeGems()
	{
		m_RegionSelectionTool->AutoRecognizeGems();
		m_RegionSelectionTool->ShowResult();
	}

	void Scene::PlaceGemsAtTargets()
	{
		GemGroup gemGroup = m_PlacementTool->PlaceGemsAtTargets();
		m_GemGroups.push_back(gemGroup);
	}

	void Scene::InteractiveSphereSelect()
	{
		m_RegionSelectionTool->InteractiveSphereSelect();
		m_RegionSelectionTool->ShowResult();
	}

	void Scene::InteractiveFillRegion()
	{
		m_RegionSelectionTool->InteractiveFillRegion();
		m_RegionSelectionTool->ShowResult();
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