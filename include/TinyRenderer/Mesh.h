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

    struct Mesh
    {
        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;
        int MaterialIndex;

        Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, int materialIndex)
        {
            Vertices = vertices;
            Indices = indices;
            MaterialIndex = materialIndex;
        }
    };

} // namespace TinyRenderer
} // namespace GemCraft