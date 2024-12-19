#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/Geodesic.h"
#include "Mesh/GemSetting.h"
#include "Mesh/GemLine.h"
#include "Panels/CustomUI.h"

namespace GemCraft {

	class Scene
	{
		friend class PreprocessTool;
		friend class PlacerTool;
		friend class BooleanTool;
	public:
		Scene();

		void OnKeyReleased(KeyCode key);
		void OnRender(const std::string& command);
		void InitGeodesic();

		void AddRing(const std::string& name, const std::string& filepath);

		std::shared_ptr<Mesh> GetRing() { return m_Ring; }

		void PreprocessRing();
	private:
		//void AdornStrokeWithGems();
		void ConstructGeodesicPath();
		void PlaceGemsOnPath();
		void BooleanOpDifference();

		void ShowRingStroke();
		void ShowRingSelected();
		void ShowSourcePoint();
		void ShowTargetPoint();

	private:
		std::shared_ptr<Mesh> m_Ring;
		std::vector<GemLine> m_GemLines;

		std::unique_ptr<Geodesic> m_Geodesic;
		std::vector<Path> m_GeodesicPaths;

		GemSelectionUI m_GemSelectionUI;
		GemSettingSelectionUI m_GemSettingSelectionUI;
		GemPatternUI m_GemPatternUI;
	};

} // namespace GemCraft