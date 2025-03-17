#include "Mesh/GeodesicTool.h"

#include "Core/Scene.h"
#include "Mesh/FormatTool.h"

namespace GemCraft {

	struct HalfedgeToEdge
	{
		HalfedgeToEdge(const CGALMesh& cgalmesh, std::vector<edge_descriptor>& edges)
			: m_CGALmesh(cgalmesh), m_Edges(edges) {}

		void operator()(const halfedge_descriptor& h) const
		{
			m_Edges.push_back(edge(h, m_CGALmesh));
		}

		const CGALMesh& m_CGALmesh;
		std::vector<edge_descriptor>& m_Edges;
	};

	GeodesicTool::GeodesicTool(std::shared_ptr<Mesh>& mesh)
	{
		GC_CORE_WARN("Initializing geodesic.");

		m_CGALmesh = FormatTool::MeshToCGALMesh(mesh, mesh->GetPsTransform());

		// Construct a shortest path query object and add a source point
		m_ShortestPaths = std::make_unique<Surface_mesh_shortest_path>(*m_CGALmesh);
		m_ShortestPaths->build_aabb_tree(m_Tree);

		GC_CORE_INFO("Completed!");
	}

	Face_location GeodesicTool::LocatePoint(CGALPoint point)
	{
		return m_ShortestPaths->locate<AABB_face_graph_traits>(point, m_Tree);
	}

	glm::vec3 GeodesicTool::CalculateNormal(const glm::vec3& position)
	{
		CGALPoint point(position.x, position.y, position.z);
		Face_location faceLoc = LocatePoint(point);

		std::vector<glm::vec3> vertices;
		CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
		for (boost::tie(vbegin, vend) = m_CGALmesh->vertices_around_face(m_CGALmesh->halfedge(faceLoc.first)); vbegin != vend; ++vbegin) {
			vertices.push_back({ m_CGALmesh->point(*vbegin).x(), m_CGALmesh->point(*vbegin).y(), m_CGALmesh->point(*vbegin).z() });
		}
		glm::vec3 normal = glm::normalize(glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]));
		return normal;
	}

	Path GeodesicTool::ConstructGeodesicPath()
	{
		// Get the source point
		glm::vec3 startPath = polyscope::state::startPath;
		Face_location startPathLoc = LocatePoint(CGALPoint(startPath.x, startPath.y, startPath.z));
		// Get the target point
		glm::vec3 endPath = polyscope::state::endPath;
		Face_location endPathLoc = LocatePoint(CGALPoint(endPath.x, endPath.y, endPath.z));

		// Compute the shortest path between the source and the target
		std::vector<CGALPoint> points;
		m_ShortestPaths->clear();
		m_ShortestPaths->add_source_point(startPathLoc.first, startPathLoc.second);
		m_ShortestPaths->shortest_path_points_to_source_points(endPathLoc.first, endPathLoc.second, std::back_inserter(points));
		Path geodesicPath;
		size_t len = points.size();
		for (size_t i = 0; i < len; i++) {
			geodesicPath.AddPoint({ points[len - 1 - i].x(), points[len - 1 - i].y(), points[len - 1 - i].z() });
		}
		return geodesicPath;
	}

} // namespace GemCraft