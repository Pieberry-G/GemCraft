#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/GemSettingType.h"

namespace GemCraft {

	class MeshResource
	{
	public:
		MeshResource(const std::string& name, std::shared_ptr<polyscope::render::TextureBuffer> icon, Mesh* mesh)
			: m_Name(name), m_Icon(icon), m_Mesh(mesh) {}

		void* GetIconTextureID() const { return m_Icon->getNativeHandle(); }
		const Mesh* GetMesh() const { return m_Mesh; }
	private:
		std::string m_Name;
		std::shared_ptr<polyscope::render::TextureBuffer> m_Icon;
		const Mesh* m_Mesh;
	};

	class ResourceManager
	{
	public:
		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;

		static ResourceManager* Get() {
			if (s_Instance == nullptr) {
				s_Instance = new ResourceManager();
			}
			return s_Instance;
		}

		std::shared_ptr<Mesh> CreateGem(const std::string& filepath);
		std::shared_ptr<Mesh> CreateGemSetting(GemSettingType settingType);

		void PreloadGems();
		void PreloadGemSettings();

		const std::unordered_map<std::string, MeshResource*>& GetGems() const { return m_GemResources; }
		const std::unordered_map<GemSettingType, MeshResource*>& GetGemSettingResources() const { return m_GemSettingResources; }
	private:
		ResourceManager() = default;

		void LoadGemSetting(GemSettingType settingType);

	private:
		std::unordered_map<std::string, MeshResource*> m_GemResources;
		std::unordered_map<GemSettingType, MeshResource*> m_GemSettingResources;
	private:
		static ResourceManager* s_Instance;
	};

} // namespace GemCraft