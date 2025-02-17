#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/GemSetting.h"

namespace GemCraft {

	class MeshResource
	{
	public:
		MeshResource(const std::string& name, std::shared_ptr<polyscope::render::TextureBuffer> icon, Mesh* gemMesh, Mesh* gemSettingMesh)
			: m_Name(name), m_Icon(icon), m_GemMesh(gemMesh), m_GemSettingMesh(gemSettingMesh) {}

		void* GetIconTextureID() const { return m_Icon->getNativeHandle(); }
		const Mesh* GetGemMesh() const { return m_GemMesh; }
		const Mesh* GetGemSettingMesh() const { return m_GemSettingMesh; }

	private:
		std::string m_Name;
		std::shared_ptr<polyscope::render::TextureBuffer> m_Icon;
		const Mesh* m_GemMesh;
		const Mesh* m_GemSettingMesh;
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

		std::shared_ptr<Mesh> CreateGem(GemSettingType settingType);
		std::shared_ptr<Mesh> CreateGemSetting(GemSettingType settingType);
		std::shared_ptr<Mesh> CreateMandrel();
		std::shared_ptr<Mesh> CreateCylinder();

		void PreloadGemSettings();
		void PreloadMandrel();
		void PreloadCylinder();

		const std::unordered_map<GemSettingType, MeshResource*>& GetResources() const { return m_Resources; }

	private:
		ResourceManager() = default;

		void LoadGemSetting(GemSettingType settingType);

	private:
		std::unordered_map<GemSettingType, MeshResource*> m_Resources;
		MeshResource* m_Mandrel;
		MeshResource* m_Cylinder;

	private:
		static ResourceManager* s_Instance;
	};

} // namespace GemCraft