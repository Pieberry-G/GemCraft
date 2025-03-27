#include "Mesh/GeodesicTool.h"

#include "Mesh/FormatTool.h"

namespace GemCraft {

	GeodesicTool::GeodesicTool(std::shared_ptr<Mesh>& mesh)
	{
		GC_CORE_WARN("Initializing geodesic.");

		m_CGALmesh = FormatTool::MeshToCGALMesh(mesh, mesh->GetPsTransform());

		// Construct a shortest path query object and add a source point
		m_ShortestPaths = std::make_unique<Surface_mesh_shortest_path>(*m_CGALmesh);
		m_ShortestPaths->build_aabb_tree(m_Tree);

		GC_CORE_INFO("Done!");
	}

	size_t GeodesicTool::QueryClosestFace(const glm::vec3& queryPoint)
	{
		CGALPoint point = { queryPoint.x, queryPoint.y, queryPoint.z };
		auto faceLoc = m_ShortestPaths->locate<AABB_face_graph_traits>(point, m_Tree);
		return faceLoc.first;
	}

	std::array<double, 3> GeodesicTool::QueryBarycentricCoords(const glm::vec3& queryPoint)
	{
		CGALPoint point = { queryPoint.x, queryPoint.y, queryPoint.z };
		auto faceLoc = m_ShortestPaths->locate<AABB_face_graph_traits>(point, m_Tree);
		return faceLoc.second;
	}

	glm::vec3 GeodesicTool::QueryClosestPoint(const glm::vec3& queryPoint)
	{
		size_t faceID = QueryClosestFace(queryPoint);
		std::array<double, 3> barycentricCoords = QueryBarycentricCoords(queryPoint);
		glm::vec3 closestPoint = { 0.0f, 0.0f, 0.0f };
		CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
		int count = 0;
		for (boost::tie(vbegin, vend) = m_CGALmesh->vertices_around_face(m_CGALmesh->halfedge(CGAL::SM_Face_index(faceID))); vbegin != vend; ++vbegin) {
			glm::vec3 v = { m_CGALmesh->point(*vbegin).x(), m_CGALmesh->point(*vbegin).y(), m_CGALmesh->point(*vbegin).z() };
			closestPoint += (float)barycentricCoords[count++] * v;
		}
		return closestPoint;
	}

	glm::vec3 GeodesicTool::CalculateNormal(const glm::vec3& position)
	{
		size_t faceID = QueryClosestFace(position);

		std::vector<glm::vec3> vertices;
		CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
		for (boost::tie(vbegin, vend) = m_CGALmesh->vertices_around_face(m_CGALmesh->halfedge(CGAL::SM_Face_index(faceID))); vbegin != vend; ++vbegin) {
			vertices.push_back({ m_CGALmesh->point(*vbegin).x(), m_CGALmesh->point(*vbegin).y(), m_CGALmesh->point(*vbegin).z() });
		}
		glm::vec3 normal = glm::normalize(glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]));
		return normal;
	}

} // namespace GemCraft