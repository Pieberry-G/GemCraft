#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace GemCraft {
namespace TinyRenderer {

    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoord;
    };

    class Mesh
    {
    public:
        Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, int materialIndex);

    public:
        std::vector<Vertex> m_Vertices;
        std::vector<uint32_t> m_Indices;
        int m_MaterialIndex;
    };

} // namespace TinyRenderer
} // namespace GemCraft