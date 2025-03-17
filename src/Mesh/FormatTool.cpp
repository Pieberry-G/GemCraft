#include "Mesh/FormatTool.h"

namespace GemCraft {
namespace FormatTool {

	std::shared_ptr<CGALMesh> MeshToCGALMesh(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform)
	{
		CGALMesh cgalmesh;
		std::vector<CGAL::SM_Vertex_index> vertices;
		for (const auto& p : mesh->GetVertices()) {
			glm::vec4 p_tf = transform * glm::vec4(p, 1.0f);
			vertices.push_back(cgalmesh.add_vertex({ p_tf.x, p_tf.y, p_tf.z }));
		}
		for (const auto& f : mesh->GetFaces()) {
			std::vector<CGAL::SM_Vertex_index> face;
			for (size_t index : f) {
				face.push_back(vertices[index]);
			}
			cgalmesh.add_face(face);
		}
		return std::make_shared<CGALMesh>(cgalmesh);
	}

	std::shared_ptr<Mesh> CGALMeshToMesh(const std::shared_ptr<CGALMesh>& cgalmesh, const glm::mat4& transform)
	{
		std::vector<glm::vec3> vertices;
		std::vector<std::vector<size_t>> faces;
		for (auto& v : cgalmesh->vertices()) {
			glm::vec4 p = { cgalmesh->point(v).x(), cgalmesh->point(v).y(), cgalmesh->point(v).z(), 1.0f };
			glm::vec4 p_tf = transform * p;
			vertices.push_back({ p_tf.x, p_tf.y, p_tf.z });
		}
		for (auto& f : cgalmesh->faces()) {
			CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
			std::vector<size_t> face;
			for (boost::tie(vbegin, vend) = cgalmesh->vertices_around_face(cgalmesh->halfedge(f)); vbegin != vend; ++vbegin) {
				face.push_back(*vbegin);
			}
			faces.push_back(face);
		}
		return std::make_shared<Mesh>("", vertices, faces);
	}

} // namespace FormatTool
} // namespace GemCraft