#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/GemSetting.h"
#include "Mesh/GemGroup.h"
#include "Panels/CustomUI.h"

#include "EventSystem/ApplicationEvent.h"
#include "EventSystem/KeyEvent.h"
#include "EventSystem/MouseEvent.h"

namespace GemCraft {

	class SurfaceCleanerTool;
	class RegionSelectionTool;
	class GeometryTool;
	class PlacementTool;
	class BooleanTool;
	class NurbsFitting;

	class Scene
	{
		friend class RegionSelectionTool;
		friend class GeometryTool;
		friend class PlacementTool;
	public:
		Scene();

		void Clean();
		bool OnKeyReleased(KeyReleasedEvent& e);
		bool OnRender(AppRenderEvent& e);

		void AddRing(const std::string& name, const std::string& filepath);
		std::string GetRingPath() { return m_RingPath; }
		std::shared_ptr<Mesh>& GetRing() { return m_Ring; }
		std::vector<GemGroup>& GetGemGroups() { return m_GemGroups; };

		void AddMesh(std::shared_ptr<Mesh>& mesh);
		std::shared_ptr<Mesh>& GetMesh(const std::string& name)
		{
			for (auto& mesh : m_Meshes) {
				if (mesh->GetName() == name) {
					return mesh;
				}
			}
		}

	private:
		void AutoSelectRegion();
		void RepairSelectedRegion();
		void DigHoleOnSelectedRegion();
		void PlaceGemsOnSelectedRegion();
		void BooleanOpDifference();

		void CleanSurface();
		void AutoRecognizeGems();
		void PlaceGemsAtTargets();

		void OnImGuizmoUsed();
		void InteractiveSphereSelect();
		void InteractiveFillRegion();
		void ShowSelectedRegion();

	private:
		std::string m_RingPath;
		std::shared_ptr<Mesh> m_Ring;
		std::vector<std::shared_ptr<Mesh>> m_Meshes;
		std::shared_ptr<NurbsFitting> m_Nurbs;

		std::vector<GemGroup> m_GemGroups;

		GemSettingSelectionUI m_GemSettingSelectionUI;
		GemPatternUI m_GemPatternUI;

		std::unique_ptr<SurfaceCleanerTool> m_SurfaceCleanerTool;
		std::unique_ptr<RegionSelectionTool> m_RegionSelectionTool;
		std::unique_ptr<GeometryTool> m_GeometryTool;
		std::unique_ptr<PlacementTool> m_PlacementTool;
		std::unique_ptr<BooleanTool> m_BooleanTool;
	};

} // namespace GemCraft