#include "Core/ResourceManager.h"

#include <stb_image.h>
#include <filesystem>

namespace GemCraft {

    ResourceManager* ResourceManager::s_Instance = nullptr;

    std::shared_ptr<Mesh> ResourceManager::CreateGem(const std::string& filepath)
    {
        if (m_GemResources.find(filepath) == m_GemResources.end()) {
            GC_CORE_WARN("Can't find a gem from resource manager!");
            return nullptr;
        }

        const Mesh& meshOrigin = *(m_GemResources.at(filepath)->GetMesh());
        std::shared_ptr<Mesh> meshCopy = std::make_shared<Mesh>(meshOrigin);

        return meshCopy;
    }

    std::shared_ptr<Mesh> ResourceManager::CreateGemSetting(GemSettingType settingType)
    {
        if (m_GemSettingResources.find(settingType) == m_GemSettingResources.end()) {
            GC_CORE_WARN("Can't find a gem base from resource manager!");
            return nullptr;
        }

        const Mesh& meshOrigin = *(m_GemSettingResources.at(settingType)->GetMesh());
        std::shared_ptr<Mesh> meshCopy = std::make_shared<Mesh>(meshOrigin);

        return meshCopy;
    }

    std::shared_ptr<Mesh> ResourceManager::CreateMandrel()
    {
        const Mesh& meshOrigin = *m_Mandrel->GetMesh();
        std::shared_ptr<Mesh> meshCopy = std::make_shared<Mesh>(meshOrigin);

        return meshCopy;
    }

    void ResourceManager::PreloadGems()
    {
        static const std::string s_GemFolder = "../assets/Gems/";

        std::filesystem::path gemFolderPath(s_GemFolder);
        if (!std::filesystem::exists(gemFolderPath) || !std::filesystem::is_directory(gemFolderPath)) {
            GC_CORE_ASSERT(false, "The specified path does not exist or is not a directory.");
        }

        for (const auto& entry : std::filesystem::directory_iterator(gemFolderPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".obj") {

                std::string filepath = entry.path().generic_string();
                std::shared_ptr<polyscope::render::TextureBuffer> icon;

                int width, height, channels;
                unsigned char* data = nullptr;
                std::string iconpath = (entry.path().parent_path() / entry.path().stem()).generic_string() + ".png";
                stbi_set_flip_vertically_on_load(1);
                data = stbi_load(iconpath.c_str(), &width, &height, &channels, 0);
                if (data) {
                    icon = polyscope::render::engine->generateTextureBuffer(polyscope::TextureFormat::RGBA8, 1028, 1028, data);
                } else {
                    GC_CORE_ASSERT(false, "Failed to load image.");
                }
                stbi_image_free(data);

                MeshResource* gem = new MeshResource("", icon, new Mesh("Gem", filepath));

                m_GemResources[filepath] = gem;
            }
        }
    }

    void ResourceManager::PreloadGemSettings()
    {
        LoadGemSetting(GemSettingType::Bezel);
        LoadGemSetting(GemSettingType::Pave);
        LoadGemSetting(GemSettingType::Prong);
        LoadGemSetting(GemSettingType::Claw);
        LoadGemSetting(GemSettingType::Invisible);
        LoadGemSetting(GemSettingType::Shovel);
        LoadGemSetting(GemSettingType::Channel);
    }

    void ResourceManager::PreloadMandrel()
    {
        Mesh* mesh = new Mesh("Mandrel", "../assets/GemSettings/Mandrel.obj");
        m_Mandrel = new MeshResource("Mandrel", nullptr, mesh);
    }

    void ResourceManager::LoadGemSetting(GemSettingType settingType)
    {
        static const std::string s_GemSettingFolder = "../assets/GemSettings/";
        std::string meshPath, iconPath;
        switch (settingType)
        {
            case GemSettingType::Invisible:
                iconPath = s_GemSettingFolder + "InvisibleSetting.png";
                break;
            case GemSettingType::Bezel:
                meshPath = s_GemSettingFolder + "BezelSetting.obj";
                iconPath = s_GemSettingFolder + "BezelSetting.png";
                break;
            case GemSettingType::Pave:
                meshPath = s_GemSettingFolder + "PaveSetting.obj";
                iconPath = s_GemSettingFolder + "PaveSetting.png";
                break;
            case GemSettingType::Prong:
                meshPath = s_GemSettingFolder + "ProngSetting.obj";
                iconPath = s_GemSettingFolder + "ProngSetting.png";
                break;
            case GemSettingType::Claw:
                meshPath = s_GemSettingFolder + "ClawSetting.obj";
                iconPath = s_GemSettingFolder + "ClawSetting.png";
                break;
            case GemSettingType::Shovel:
                meshPath = s_GemSettingFolder + "ShovelSetting.obj";
                iconPath = s_GemSettingFolder + "ShovelSetting.png";
                break;
            case GemSettingType::Channel:
                iconPath = s_GemSettingFolder + "ChannelSetting.png";
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

        Mesh* mesh;
        switch (settingType)
        {
            case GemSettingType::Invisible:
            case GemSettingType::Channel:
                mesh = new Mesh("GemSetting", std::vector<glm::vec3>(), std::vector<std::vector<size_t>>());
                break;
            default:
                mesh = new Mesh("GemSetting", meshPath);
        }
        m_GemSettingResources[settingType] = new MeshResource(GetName(settingType), icon, mesh);
    }

} // namespace GemCraft