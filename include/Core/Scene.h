#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/GeodesicTool.h"
#include "Mesh/GemSetting.h"
#include "Mesh/GemGroup.h"
#include "Panels/CustomUI.h"

namespace GemCraft {

	class Scene
	{
		friend class PlacementTool;
		friend class BooleanTool;
	public:
		Scene();

		void OnKeyReleased(KeyCode key);
		void OnRender(const std::string& command);
		void InitGeodesic();

		void AddRing(const std::string& name, const std::string& filepath);
		std::shared_ptr<Mesh>& GetRing() { return m_Ring; }
		std::string GetRingPath() { return m_RingPath; }

		void AddMesh(std::shared_ptr<Mesh>& mesh);
		std::shared_ptr<Mesh>& GetMesh(const std::string& name)
		{
			for (auto& mesh : m_Meshes) {
				if (mesh->GetName() == name) {
					return mesh;
				}
			}
		}

		void CleanSurface();
		void AutoRecognizeGems();
		void AutoSelectRegion();
		void RepairSelectedRegion();
		void PlaceGemsAtTargets();

	private:
		void ConstructGeodesicPath();
		void PlaceGemsOnPath();
		void PlaceGemsOnSelectedRegion();
		void BooleanOpDifference();

		void ShowRingStroke();
		void ShowSelectedRegion();
		void ShowSourcePoint();
		void ShowTargetPoint();

	private:
		std::string m_RingPath;
		std::shared_ptr<Mesh> m_Ring;
		std::vector<std::shared_ptr<Mesh>> m_Meshes;

		std::vector<GemLine> m_GemLines;
		std::vector<GemGroup> m_GemGroups;

		std::unique_ptr<GeodesicTool> m_GeodesicTool;
		Path m_GeodesicPath;

		GemSettingSelectionUI m_GemSettingSelectionUI;
		GemPatternUI m_GemPatternUI;
	};

} // namespace GemCraft