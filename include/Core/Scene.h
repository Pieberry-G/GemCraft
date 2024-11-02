#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/Geodesic.h"
#include "Mesh/GemSettingType.h"
#include "Panels/CustomUI.h"

namespace GemCraft {

	class Scene
	{
	public:
		Scene();

		void OnKeyReleased(KeyCode key);
		void OnRender(const std::string& command);
		void InitGeodesic();

		void AddRing(const std::string& name, const std::string& filepath);
		void AddGem(const std::string& name, const std::string& filepath, const glm::mat4& transform = glm::mat4(1.0f));
		void AddGem(const std::string& name, const std::string& filepath, const glm::vec3& position, const glm::vec3& forward, const glm::vec3& direction);

		void AddGemSetting(const std::string& name, GemSettingType gemSetting, const glm::mat4& transform = glm::mat4(1.0f));
		void AddGemSetting(const std::string& name, GemSettingType gemSetting, const glm::vec3& position, const glm::vec3& forward, const glm::vec3& direction);

		std::shared_ptr<Mesh> GetRing() { return m_Ring; }

		void AdornStrokeWithGems();
		void PlaceGemsOnPath();
		void BooleanOpDifference();
		void ConstructGeodesicPath();

	private:
		void ShowRingStroke();
		void ShowRingSelected();
		void ShowSourcePoint();
		void ShowTargetPoint();

	private:
		std::shared_ptr<Mesh> m_Ring;
		std::vector<std::shared_ptr<Mesh>> m_Gems;
		std::vector<std::shared_ptr<Mesh>> m_GemSettings;

		std::unique_ptr<Geodesic> m_Geodesic;
		std::vector<Path> m_GeodesicPaths;

		GemSelectionUI m_GemSelectionUI;
		GemSettingSelectionUI m_GemSettingSelectionUI;
		GemPatternUI m_GemPatternUI;
	};

} // namespace GemCraft