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
		std::tie(m_GCTmesh, m_GCTgeo) = FormatTool::MeshToGCTMesh(mesh);

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

	glm::vec3 GeodesicTool::TracePath(Face_location faceLoc, const glm::vec3& forward, float distance)
	{
		GCTsf::VertexData<GCT::Vector3>& vertexPositions = m_GCTgeo->inputVertexPositions;

		GCTsf::Face face(m_GCTmesh.get(), faceLoc.first.id());
		GCT::Vector3 faceCoords({ faceLoc.second[1], faceLoc.second[2], faceLoc.second[0] });
		GCTsf::SurfacePoint traceOrigin(face, faceCoords);

		std::vector<GCT::Vector3> vertices;
		CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
		for (boost::tie(vbegin, vend) = m_CGALmesh->vertices_around_face(m_CGALmesh->halfedge(faceLoc.first)); vbegin != vend; ++vbegin) {
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

	std::vector<Path> GeodesicTool::CalculateParallelPaths(const Path& path, int numberOfPaths, float pathSpacing)
	{
		const std::vector<glm::vec3>& points = path.Points();

		// Calculate forward direction
		size_t len = path.Length();
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
				parallelPaths.push_back(path);
				continue;
			}
			Path parallelPath;
			for (size_t i = 0; i < len; i++) {
				CGALPoint start_point(points[i].x, points[i].y, points[i].z);
				Face_location startPointLoc = LocatePoint(start_point);
				parallelPath.AddPoint(TracePath(startPointLoc, forwardDirections[i], k * pathSpacing));
			}
			parallelPaths.push_back(parallelPath);
		}
		return parallelPaths;
	}

} // namespace GemCraft