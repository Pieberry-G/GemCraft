#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/GemSetting.h"

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
		std::shared_ptr<Mesh> CreateMandrel();
		std::shared_ptr<Mesh> CreateCylinder();

		void PreloadGems();
		void PreloadGemSettings();
		void PreloadMandrel();
		void PreloadCylinder();

		const std::unordered_map<std::string, MeshResource*>& GetGems() const { return m_GemResources; }
		const std::unordered_map<GemSettingType, MeshResource*>& GetGemSettingResources() const { return m_GemSettingResources; }
	private:
		ResourceManager() = default;

		void LoadGemSetting(GemSettingType settingType);

	private:
		std::unordered_map<std::string, MeshResource*> m_GemResources;
		std::unordered_map<GemSettingType, MeshResource*> m_GemSettingResources;
		MeshResource* m_Mandrel;
		MeshResource* m_Cylinder;

	private:
		static ResourceManager* s_Instance;
	};

} // namespace GemCraft