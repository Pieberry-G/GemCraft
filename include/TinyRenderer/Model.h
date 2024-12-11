#pragma once

#include "TinyRenderer/Mesh.h"
#include "TinyRenderer/Camera.h"
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
    public:
        Model(const std::string& filename);

        void Draw(Camera& camera, std::shared_ptr<polyscope::render::ShaderProgram> gltfShaderProgram);
        void SaveRenderResult(const std::string& outFile, uint32_t location = 0);

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
        std::shared_ptr<Image> m_WhiteTexture;
        std::vector<std::shared_ptr<Image>> m_Images;
        std::vector<Texture> m_Textures;
        std::vector<Material> m_Materials;
        std::vector<Mesh> m_Meshes;

        glm::vec3 m_MinBounds, m_MaxBounds;
    };

} // namespace TinyRenderer
} // namespace GemCraft