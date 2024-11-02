#pragma once

#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"

namespace GemCraft {

    class Mesh
    {
    public:
        Mesh(const std::string& name, const std::string& path, bool isRing = false);
        Mesh(const std::string& name, const std::vector<glm::vec3>& vertices, const std::vector<std::vector<size_t>>& faces);

        // Count
        size_t nVertices() const { return  m_Vertices.size(); }
        size_t nFaces() const { return  m_Faces.size(); }
        size_t nEdges() const { return  m_Faces.size() * 3; }

        // Get
        const std::vector<glm::vec3>& GetVertices() const { return m_Vertices; }
        const std::vector<std::vector<size_t>>& GetFaces() const { return m_Faces; }
        polyscope::SurfaceMesh* GetPsMesh() const { return m_PsMesh; }
        glm::mat4 GetPsTransform() const { return m_PsMesh ? m_PsMesh->getTransform() : glm::mat4(1.0f); }
        bool GetBooleanOpLock() const { return m_BooleanOpLock; }

        // Set
        void SetName(const std::string& name) { m_Name = name; }
        void SetBooleanOpLock(bool booleanOpLock) { m_BooleanOpLock = booleanOpLock; }

        // polyscope
        void AddToPolyscope(const glm::mat4& transform = glm::mat4(1.0f));
        void RemoveFromPolyscope();
        void SetPsMeshSurfaceColor(const glm::vec3& color);
    private:
        std::string m_Name;

        std::vector<glm::vec3> m_Vertices;
        std::vector<std::vector<size_t>> m_Faces;

        polyscope::SurfaceMesh* m_PsMesh = nullptr;

        bool m_BooleanOpLock = false;
    };

} // namespace GemCraft