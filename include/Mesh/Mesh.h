#pragma once

#include <polyscope/polyscope.h>
#include <polyscope/surface_mesh.h>

namespace GemCraft {

    class Mesh
    {
    public:
        Mesh(const std::string& name, const std::string& filepath);
        Mesh(const std::string& name, const std::vector<glm::vec3>& vertices, const std::vector<std::vector<size_t>>& faces);

        std::string GetFilepath() const { return m_Filepath; }

        size_t nVertices() const { return  m_Vertices.size(); }
        size_t nFaces() const { return  m_Faces.size(); }
        size_t nEdges() const { return  m_Faces.size() * 3; }

        const std::vector<glm::vec3>& GetVertices() const { return m_Vertices; }
        const std::vector<std::vector<size_t>>& GetFaces() const { return m_Faces; }
        polyscope::SurfaceMesh* GetPsMesh() const { return m_PsMesh; }
        glm::mat4 GetPsTransform() const { return m_PsMesh ? m_PsMesh->getTransform() : glm::mat4(1.0f); }
        std::string GetName() { return m_Name; }

        void SetName(const std::string& name) { m_Name = name; }

        // polyscope
        void AddToPolyscope(const glm::mat4& transform = glm::mat4(1.0f));
        void RemoveFromPolyscope();
        void SetPsMeshSurfaceColor(const glm::vec3& color);

    private:
        std::string m_Name;
        std::string m_Filepath;

        std::vector<glm::vec3> m_Vertices;
        std::vector<std::vector<size_t>> m_Faces;

        polyscope::SurfaceMesh* m_PsMesh = nullptr;
    };

} // namespace GemCraft