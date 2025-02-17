#include "Mesh/Mesh.h"

#include <polyscope/polyscope.h>
#include <polyscope/surface_mesh.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel     Kernel;
typedef Kernel::Point_3                                         CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>                           CGALMesh;
typedef boost::graph_traits<CGALMesh>::halfedge_descriptor      halfedge_descriptor;
typedef boost::graph_traits<CGALMesh>::edge_descriptor          edge_descriptor;
typedef boost::graph_traits<CGALMesh>::face_descriptor          face_descriptor;

namespace CGALpmp = CGAL::Polygon_mesh_processing;

namespace GemCraft {

    Mesh::Mesh(const std::string& name, const std::string& filepath)
        : m_Name(name)
    {
        CGALMesh cgalmesh;
        if (!CGALpmp::IO::read_polygon_mesh(filepath, cgalmesh)) {
            GC_CORE_ASSERT(false, "Can not load mesh!");
        }

        //// Triangulate mesh
        //CGALpmp::triangulate_faces(mesh);

        for (auto& v : cgalmesh.vertices()) {
            m_Vertices.push_back({ cgalmesh.point(v).x(), cgalmesh.point(v).y(), cgalmesh.point(v).z() });
        }
        for (auto& f : cgalmesh.faces()) {
            CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
            std::vector<size_t> face;
            for (boost::tie(vbegin, vend) = cgalmesh.vertices_around_face(cgalmesh.halfedge(f)); vbegin != vend; ++vbegin) {
                face.push_back(*vbegin);
            }
            m_Faces.push_back(face);
        }
    }

    Mesh::Mesh(const std::string& name, const std::vector<glm::vec3>& vertices, const std::vector<std::vector<size_t>>& faces)
        : m_Name(name), m_Vertices(vertices), m_Faces(faces) {}

    void Mesh::AddToPolyscope(const glm::mat4& transform)
    {
        GC_CORE_ASSERT(!m_PsMesh, "Mesh has already been registered to polyscope!");

        m_PsMesh = polyscope::registerSurfaceMesh(m_Name, m_Vertices, m_Faces);
        m_PsMesh->setTransform(transform);
    }

    void Mesh::RemoveFromPolyscope()
    {
        GC_CORE_ASSERT(m_PsMesh, "Mesh has not been registered to polyscope!");

        m_PsMesh->remove();
        m_PsMesh = nullptr;
    }

    void Mesh::SetPsMeshSurfaceColor(const glm::vec3& color)
    {
        GC_CORE_ASSERT(m_PsMesh, "Mesh has not been registered to polyscope!");

        m_PsMesh->setSurfaceColor(color);
    }

} // namespace GemCraft