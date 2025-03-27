#include "Mesh/PaddingMesh.h"

#include "Mesh/FormatTool.h"

namespace GemCraft {

	void PaddingMesh::CreatePaddingMesh(const glm::vec3& basePoint, float length, float width, float height, float radius)
	{
		glm::vec3 center = basePoint - glm::vec3(0.0f, radius, 0.0f);
		float totalRadians = length / radius;
		int numPoints = 100;
		std::vector<glm::vec3> points;
		for (int i = 0; i < numPoints; i++) {
			float angle = -totalRadians / 2 + totalRadians * i / (numPoints - 1);
			glm::vec3 point(center.x + radius * glm::sin(angle), center.y + radius * glm::cos(angle), 0.0f);
			points.push_back(point);
		}

		std::shared_ptr<CGALMesh> cgalPaddingMesh = std::make_shared<CGALMesh>();
		for (int i = 0; i < numPoints; i++) {
			float angle = -totalRadians / 2 + totalRadians * i / (numPoints - 1);
			cgalPaddingMesh->add_vertex(CGALPoint(center.x + radius * glm::sin(angle), center.y + radius * glm::cos(angle), width / 2.0f));
			cgalPaddingMesh->add_vertex(CGALPoint(center.x + (radius - height) * glm::sin(angle), center.y + (radius - height) * glm::cos(angle), width / 2.0f));
			cgalPaddingMesh->add_vertex(CGALPoint(center.x + radius * glm::sin(angle), center.y + radius * glm::cos(angle), -width / 2.0f));
			cgalPaddingMesh->add_vertex(CGALPoint(center.x + (radius - height) * glm::sin(angle), center.y + (radius - height) * glm::cos(angle), -width / 2.0f));
		}
		for (int i = 0; i < numPoints - 1; i++) {
			cgalPaddingMesh->add_face(CGALMesh::vertex_index(4 * i), CGALMesh::vertex_index(4 * i + 5), CGALMesh::vertex_index(4 * i + 4));
			cgalPaddingMesh->add_face(CGALMesh::vertex_index(4 * i), CGALMesh::vertex_index(4 * i + 1), CGALMesh::vertex_index(4 * i + 5));
			cgalPaddingMesh->add_face(CGALMesh::vertex_index(4 * i + 2), CGALMesh::vertex_index(4 * i + 6), CGALMesh::vertex_index(4 * i + 7));
			cgalPaddingMesh->add_face(CGALMesh::vertex_index(4 * i + 2), CGALMesh::vertex_index(4 * i + 7), CGALMesh::vertex_index(4 * i + 3));

			cgalPaddingMesh->add_face(CGALMesh::vertex_index(4 * i), CGALMesh::vertex_index(4 * i + 6), CGALMesh::vertex_index(4 * i + 2));
			cgalPaddingMesh->add_face(CGALMesh::vertex_index(4 * i), CGALMesh::vertex_index(4 * i + 4), CGALMesh::vertex_index(4 * i + 6));
			cgalPaddingMesh->add_face(CGALMesh::vertex_index(4 * i + 1), CGALMesh::vertex_index(4 * i + 3), CGALMesh::vertex_index(4 * i + 7));
			cgalPaddingMesh->add_face(CGALMesh::vertex_index(4 * i + 1), CGALMesh::vertex_index(4 * i + 7), CGALMesh::vertex_index(4 * i + 5));
		}
		cgalPaddingMesh->add_face(CGALMesh::vertex_index(0), CGALMesh::vertex_index(2), CGALMesh::vertex_index(3));
		cgalPaddingMesh->add_face(CGALMesh::vertex_index(0), CGALMesh::vertex_index(3), CGALMesh::vertex_index(1));
		cgalPaddingMesh->add_face(CGALMesh::vertex_index(4 * (numPoints - 1)), CGALMesh::vertex_index(4 * (numPoints - 1) + 1), CGALMesh::vertex_index(4 * (numPoints - 1) + 3));
		cgalPaddingMesh->add_face(CGALMesh::vertex_index(4 * (numPoints - 1)), CGALMesh::vertex_index(4 * (numPoints - 1) + 3), CGALMesh::vertex_index(4 * (numPoints - 1) + 2));

		double targetEdgeLength = 0.1;
		std::vector<edge_descriptor> border;
		CGALpmp::border_halfedges(faces(*cgalPaddingMesh), *cgalPaddingMesh, boost::make_function_output_iterator([&](const halfedge_descriptor& h) {
			border.push_back(edge(h, *cgalPaddingMesh));
		}));
		CGALpmp::split_long_edges(border, targetEdgeLength, *cgalPaddingMesh);
		CGALpmp::isotropic_remeshing(faces(*cgalPaddingMesh), targetEdgeLength, *cgalPaddingMesh,
			CGAL::parameters::number_of_iterations(10)
			.protect_constraints(true));
		cgalPaddingMesh->collect_garbage();


		std::ofstream out("padding_mesh.obj");
		CGAL::IO::write_OBJ(out, *cgalPaddingMesh);
		out.close();

		m_PaddingMesh = FormatTool::CGALMeshToMesh(cgalPaddingMesh, glm::mat4(1.0f));
		m_PaddingMesh->SetName("Boolean Mesh");

		m_PaddingMesh->AddToPolyscope();
	}

} // namespace GemCraft