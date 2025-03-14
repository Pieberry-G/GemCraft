#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/Path.h"
#include "Mesh/GemGroup.h"
#include "Mesh/RegionSubmesh.h"															        

namespace GemCraft {

	struct GemSpecification
	{
		std::string Name;
		GemSettingType SettingType;
		glm::vec3 Position;
		glm::vec3 Forward;
		glm::vec3 Normal;
		float ExposureDepth;
		float Scale;
	};

	struct GemSettingSpecification
	{
		std::string Name;
		GemSettingType SettingType;
		glm::vec3 Position;
		glm::vec3 Forward;
		glm::vec3 Normal;
		float ExposureDepth;
		float Scale;
	};

	class Scene;
	class PlacementTool
	{
	public:
		PlacementTool(Scene* scene)
			: m_Scene(scene) {}

		void Clean();

		void BuildSubmeshForSelectedRegion();
		std::shared_ptr<Mesh> CreateMeshForBooleanHole();

		GemGroup PlaceGemsOnSelectedRegion();
		GemGroup PlaceGemsAtTargets();

		void ShowResult();
		void ShowBooleanMesh();

	private:
		std::shared_ptr<Mesh> PlaceGem(const std::string& name, GemSettingType settingType, const glm::mat4& transform = glm::mat4(1.0f));
		std::shared_ptr<Mesh> PlaceGem(GemSpecification spec);

		std::shared_ptr<Mesh> PlaceGemSetting(const std::string& name, GemSettingType settingType, const glm::mat4& transform = glm::mat4(1.0f));
		std::shared_ptr<Mesh> PlaceGemSetting(GemSettingSpecification spec);
	
		GemGroup PlaceGemsOnPositions(const std::vector<glm::vec3>& positions);

	private:
		std::shared_ptr<RegionSubmesh> m_Submesh;

	private:
		Scene* m_Scene;
	};

} // namespace GemCraft