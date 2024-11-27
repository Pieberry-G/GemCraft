#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/Path.h"
#include "Mesh/GemLine.h"

namespace GemCraft {

	struct GemSpecification
	{
		std::string Name;
		std::string Filepath;
		glm::vec3 Position;
		glm::vec3 Forward;
		glm::vec3 Normal;
		float ExposureDepth;
		float Scale;
		float Rotation;
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
		float Rotation;
	};

	class Scene;
	class PlacerTool
	{
	public:
		PlacerTool(Scene* scene)
			: m_Scene(scene) {}

		GemLine PlaceGemsOnPath(const Path& path);

	private:
		std::shared_ptr<Mesh> PlaceGem(const std::string& name, const std::string& filepath, const glm::mat4& transform = glm::mat4(1.0f));
		std::shared_ptr<Mesh> PlaceGem(GemSpecification spec);

		std::shared_ptr<Mesh> PlaceGemSetting(const std::string& name, GemSettingType settingType, const glm::mat4& transform = glm::mat4(1.0f));
		std::shared_ptr<Mesh> PlaceGemSetting(GemSettingSpecification spec);
	
	public:
		Scene* m_Scene;
	};

} // namespace GemCraft