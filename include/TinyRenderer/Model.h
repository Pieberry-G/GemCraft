#pragma once

#include "TinyRenderer/Mesh.h"
#include "TinyRenderer/Image.h"

#include <polyscope/polyscope.h>
#include <tiny_gltf.h>

namespace GemCraft {
namespace TinyRenderer {

    struct Material
    {
        glm::vec4 BaseColorFactor = glm::vec4(1.0f);
        int BaseColorTextureIndex = -1;
    };

    struct Texture
    {
        uint32_t ImageIndex;
    };

    class Model
    {
        friend class RenderTool;
    public:
        Model(const std::string& filename);

        glm::vec3 GetCenter() { return (m_MaxBounds + m_MinBounds) / 2.0f; }
        float GetRadius() { return glm::distance(m_MaxBounds, m_MinBounds) / 2.0f; }

    private:
        void LoadOBJFile(const std::string& filename);
        void LoadGLTFFile(const std::string& filename);
        void LoadGLBFile(const std::string& filename);

        // GLTF/GLB Loader Tool
        void LoadGLTFImages(tinygltf::Model& input);
        void LoadGLTFTextures(tinygltf::Model& input);
        void LoadGLTFMaterials(tinygltf::Model& input);
        void LoadGLTFNode(const tinygltf::Node& inputNode, const tinygltf::Model& input);

        void ComputeBounds();

    private:
        std::vector<std::shared_ptr<Image>> m_Images;
        std::vector<Texture> m_Textures;
        std::vector<Material> m_Materials;
        std::vector<Mesh> m_Meshes;

        glm::vec3 m_MinBounds, m_MaxBounds;
    };

} // namespace TinyRenderer
} // namespace GemCraft