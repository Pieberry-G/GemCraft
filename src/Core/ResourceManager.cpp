#include "Core/ResourceManager.h"

#include <stb_image.h>
#include <filesystem>

namespace GemCraft {

    ResourceManager* ResourceManager::s_Instance = nullptr;

    std::shared_ptr<Mesh> ResourceManager::CreateGem(GemSettingType settingType)
    {
        if (m_Resources.find(settingType) == m_Resources.end()) {
            GC_CORE_WARN("Can't find a gem base from resource manager!");
            return nullptr;
        }

        const Mesh& meshOrigin = *(m_Resources.at(settingType)->GetGemMesh());
        std::shared_ptr<Mesh> meshCopy = std::make_shared<Mesh>(meshOrigin);

        return meshCopy;
    }

    std::shared_ptr<Mesh> ResourceManager::CreateGemSetting(GemSettingType settingType)
    {
        if (m_Resources.find(settingType) == m_Resources.end()) {
            GC_CORE_WARN("Can't find a gem base from resource manager!");
            return nullptr;
        }

        const Mesh& meshOrigin = *(m_Resources.at(settingType)->GetGemSettingMesh());
        std::shared_ptr<Mesh> meshCopy = std::make_shared<Mesh>(meshOrigin);

        return meshCopy;
    }

    std::shared_ptr<Mesh> ResourceManager::CreateMandrel()
    {
        const Mesh& meshOrigin = *m_Mandrel->GetGemMesh();
        std::shared_ptr<Mesh> meshCopy = std::make_shared<Mesh>(meshOrigin);

        return meshCopy;
    }

    std::shared_ptr<Mesh> ResourceManager::CreateCylinder()
    {
        const Mesh& meshOrigin = *m_Cylinder->GetGemMesh();
        std::shared_ptr<Mesh> meshCopy = std::make_shared<Mesh>(meshOrigin);

        return meshCopy;
    }

    void ResourceManager::PreloadGemSettings()
    {
        LoadGemSetting(GemSettingType::Bezel);
        LoadGemSetting(GemSettingType::Pave);
        LoadGemSetting(GemSettingType::Prong);
        LoadGemSetting(GemSettingType::Claw);
        LoadGemSetting(GemSettingType::Invisible);
        //LoadGemSetting(GemSettingType::Shovel);
        //LoadGemSetting(GemSettingType::Channel);
        LoadGemSetting(GemSettingType::Gen003);
    }

    void ResourceManager::PreloadMandrel()
    {
        Mesh* mesh = new Mesh("Mandrel", "../assets/meshes/GemSettings/Mandrel.obj");
        m_Mandrel = new MeshResource("Mandrel", nullptr, mesh, mesh);
    }

    void ResourceManager::PreloadCylinder()
    {
        Mesh* mesh = new Mesh("Cylinder", "../assets/meshes/GemSettings/Cylinder.obj");
        m_Cylinder = new MeshResource("Cylinder", nullptr, mesh, mesh);
    }

    void ResourceManager::LoadGemSetting(GemSettingType settingType)
    {
        static const std::string s_GemSettingFolder = "../assets/meshes/GemSettings/";
        std::string gemMeshPath, gemSettingMeshPath, iconPath;
        switch (settingType)
        {
            case GemSettingType::Invisible:
                gemMeshPath = s_GemSettingFolder + "RoundGem1.obj";
                gemSettingMeshPath = "";
                iconPath = s_GemSettingFolder + "InvisibleSetting.png";
                break;
            case GemSettingType::Bezel:
                gemMeshPath = s_GemSettingFolder + "RoundGem1.obj";
                gemSettingMeshPath = s_GemSettingFolder + "BezelSetting.obj";
                iconPath = s_GemSettingFolder + "BezelSetting.png";
                break;
            case GemSettingType::Pave:
                gemMeshPath = s_GemSettingFolder + "RoundGem1.obj";
                gemSettingMeshPath = s_GemSettingFolder + "PaveSetting.obj";
                iconPath = s_GemSettingFolder + "PaveSetting.png";
                break;
            case GemSettingType::Prong:
                gemMeshPath = s_GemSettingFolder + "RoundGem1.obj";
                gemSettingMeshPath = s_GemSettingFolder + "ProngSetting.obj";
                iconPath = s_GemSettingFolder + "ProngSetting.png";
                break;
            case GemSettingType::Claw:
                gemMeshPath = s_GemSettingFolder + "RoundGem1.obj";
                gemSettingMeshPath = s_GemSettingFolder + "ClawSetting.obj";
                iconPath = s_GemSettingFolder + "ClawSetting.png";
                break;
            //case GemSettingType::Shovel:
            //    gemMeshPath = s_GemSettingFolder + "RoundGem1.obj";
            //    gemSettingMeshPath = s_GemSettingFolder + "ShovelSetting.obj";
            //    iconPath = s_GemSettingFolder + "ShovelSetting.png";
            //    break;
            //case GemSettingType::Channel:
            //    gemMeshPath = s_GemSettingFolder + "RoundGem1.obj";
            //    gemSettingMeshPath = "";
            //    iconPath = s_GemSettingFolder + "ChannelSetting.png";
            //    break;
            case GemSettingType::Gen003:
                gemMeshPath = s_GemSettingFolder + "Gen003Gem.obj";
                gemSettingMeshPath = s_GemSettingFolder + "Gen003Setting.obj";
                iconPath = s_GemSettingFolder + "ClawSetting.png";
                break;
        }

        std::shared_ptr<polyscope::render::TextureBuffer> icon;
        int width, height, channels;
        unsigned char* data = nullptr;
        stbi_set_flip_vertically_on_load(1);
        data = stbi_load(iconPath.c_str(), &width, &height, &channels, 0);
        if (data) {
            icon = polyscope::render::engine->generateTextureBuffer(polyscope::TextureFormat::RGBA8, 1028, 1028, data);
        } else {
            GC_CORE_ASSERT(false, "Failed to load image.");
        }
        stbi_image_free(data);

        Mesh* gemMesh = new Mesh("Gem", gemMeshPath);
        Mesh* gemSettingMesh;
        gemSettingMesh = gemSettingMeshPath != "" ? new Mesh("GemSetting", gemSettingMeshPath) : 
            new Mesh("GemSetting", std::vector<glm::vec3>(), std::vector<std::vector<size_t>>());
        m_Resources[settingType] = new MeshResource(GetName(settingType), icon, gemMesh, gemSettingMesh);
    }

} // namespace GemCraft