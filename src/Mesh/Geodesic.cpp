#include "Mesh/Geodesic.h"
#include "Mesh/Utils.h"

namespace GemCraft {

	Geodesic::Geodesic(std::shared_ptr<Mesh>& mesh)
	{
		m_Mesh = mesh;
		m_CGALmesh = Utils::MeshToCGALMesh(m_Mesh);
		std::tie(m_GCTmesh, m_GCTgeo) = Utils::MeshToGCTMesh(m_Mesh);

		// Construct a shortest path query object and add a source point
		m_ShortestPaths = std::make_unique<Surface_mesh_shortest_path>(*m_CGALmesh);
		m_ShortestPaths->build_aabb_tree(m_Tree);
	}

	glm::vec3 Geodesic::CalculateNormal(const glm::vec3& position)
	{
		CGALPoint point(position.x, position.y, position.z);
		Face_location face_loc = m_ShortestPaths->locate<AABB_face_graph_traits>(point, m_Tree);

		std::vector<glm::vec3> vertices;
		CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
		for (boost::tie(vbegin, vend) = m_CGALmesh->vertices_around_face(m_CGALmesh->halfedge(face_loc.first)); vbegin != vend; ++vbegin) {
			vertices.push_back({ m_CGALmesh->point(*vbegin).x(), m_CGALmesh->point(*vbegin).y(), m_CGALmesh->point(*vbegin).z() });
		}
		glm::vec3 normal = glm::normalize(glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]));
		return normal;
	}

	Path Geodesic::ConstructGeodesicPath()
	{
		// Get the source point
		glm::vec3 startPath = polyscope::state::startPath;
		CGALPoint start_path(startPath.x, startPath.y, startPath.z);
		Face_location start_path_loc = m_ShortestPaths->locate<AABB_face_graph_traits>(start_path, m_Tree);
		// Get the target point
		glm::vec3 endPath = polyscope::state::endPath;
		CGALPoint end_path(endPath.x, endPath.y, endPath.z);
		Face_location end_path_loc = m_ShortestPaths->locate<AABB_face_graph_traits>(end_path, m_Tree);

		// Compute the shortest path between the source and the target
		std::vector<CGALPoint> points;
		m_ShortestPaths->clear();
		m_ShortestPaths->add_source_point(start_path_loc.first, start_path_loc.second);
		m_ShortestPaths->shortest_path_points_to_source_points(end_path_loc.first, end_path_loc.second, std::back_inserter(points));
		Path geodesicPath;
		size_t len = points.size();
		for (size_t i = 0; i < len; i++) {
			geodesicPath.AddPoint({ points[len - 1 - i].x(), points[len - 1 - i].y(), points[len - 1 - i].z() });
		}
		return geodesicPath;
	}

	std::vector<Path> Geodesic::CalculateParallelPaths(const Path& geodesicPath, int numberOfPaths, float pathSpacing)
	{
		const std::vector<glm::vec3>& points = geodesicPath.Points();

		// Calculate forward direction
		size_t len = geodesicPath.Length();
		std::vector<glm::vec3> forwardDirections;
		forwardDirections.push_back(glm::normalize(points[1] - points[0]));
		for (size_t i = 1; i < len - 1; i++) {
			forwardDirections.push_back(glm::normalize(points[i + 1] - points[i - 1]));
		}
		forwardDirections.push_back(glm::normalize(points[len - 1] - points[len - 2]));

		// Trace path from every point of geodesic path
		std::vector<Path> parallelPaths;
		for (size_t count = 0; count < numberOfPaths; count++) {
			float k = (float)count - (numberOfPaths - 1) / 2.0f;
			if (k == 0.0f) {
				parallelPaths.push_back(geodesicPath);
				continue;
			}
			Path parallelPath;
			for (size_t i = 0; i < len; i++) {
				CGALPoint start_point(points[i].x, points[i].y, points[i].z);
				Face_location start_point_loc = m_ShortestPaths->locate<AABB_face_graph_traits>(start_point, m_Tree);
				parallelPath.AddPoint(TracePath(start_point_loc, forwardDirections[i], k * pathSpacing));
			}
			parallelPaths.push_back(parallelPath);
		}
		return parallelPaths;
	}

	glm::vec3 Geodesic::TracePath(Face_location face_loc, const glm::vec3& forward, float distance)
	{
		GCTsf::VertexData<GCT::Vector3>& vertexPositions = m_GCTgeo->inputVertexPositions;

		GCTsf::Face face(m_GCTmesh.get(), face_loc.first.id());
		GCT::Vector3 faceCoords({ face_loc.second[1], face_loc.second[2], face_loc.second[0] });
		GCTsf::SurfacePoint traceOrigin(face, faceCoords);

		std::vector<GCT::Vector3> vertices;
		CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
		for (boost::tie(vbegin, vend) = m_CGALmesh->vertices_around_face(m_CGALmesh->halfedge(face_loc.first)); vbegin != vend; ++vbegin) {
			vertices.push_back({ m_CGALmesh->point(*vbegin).x(), m_CGALmesh->point(*vbegin).y(), m_CGALmesh->point(*vbegin).z() });
		}
		GCT::Vector3 normal = GCT::normalize(GCT::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]));
		GCT::Vector3 right = GCT::cross(GCT::Vector3{ forward.x, forward.y, forward.z }, normal);
		GCT::Vector3 xDir = GCT::normalize(vertexPositions[face.halfedge().tipVertex()] - vertexPositions[face.halfedge().tailVertex()]);
		double angle = GCT::angle(xDir, right);
		if (GCT::dot(GCT::cross(xDir, right), normal) < 0) {
			angle = -angle;
		}
		GCT::Vector2 traceVec = distance * GCT::Vector2::fromAngle(angle);
		GCTsf::SurfacePoint vEnd = GCTsf::traceGeodesic(*m_GCTgeo, traceOrigin, traceVec).endPoint;
		GCT::Vector3 traceResult = vEnd.interpolate(vertexPositions);
		return { traceResult.x, traceResult.y, traceResult.z };
	}

} // namespace GemCraft