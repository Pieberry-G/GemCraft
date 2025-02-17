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

	GeodesicTool::GeodesicTool(std::shared_ptr<Mesh>& mesh, Scene* scene)
		: m_Scene(scene)
	{
		GC_CORE_WARN("Initializing geodesic.");

		m_CGALmesh = FormatTool::MeshToCGALMesh(mesh, mesh->GetPsTransform());
		double target_edge_length = 0.7;
		unsigned int nb_iter = 10;

		std::vector<edge_descriptor> border;
		CGALpmp::border_halfedges(faces(*m_CGALmesh), *m_CGALmesh, boost::make_function_output_iterator(HalfedgeToEdge(*m_CGALmesh, border)));
		CGALpmp::split_long_edges(border, target_edge_length, *m_CGALmesh);

		CGALMesh::Property_map<edge_descriptor, bool> edge_is_sharp =
			m_CGALmesh->add_property_map<edge_descriptor, bool>("e:sharp", false).first;
		CGAL::Polygon_mesh_processing::detect_sharp_edges(*m_CGALmesh, 15.0, edge_is_sharp);

		CGALpmp::isotropic_remeshing(faces(*m_CGALmesh), target_edge_length, *m_CGALmesh,
			CGAL::parameters::number_of_iterations(nb_iter)
			.edge_is_constrained_map(edge_is_sharp)
			.protect_constraints(true)); //i.e. protect border, here

		CGAL::IO::write_polygon_mesh("remeshed.obj", *m_CGALmesh, CGAL::parameters::stream_precision(17));
		std::shared_ptr<Mesh> remeshedMesh = std::make_shared<Mesh>("Remeshed Result", "remeshed.obj");

		//m_CGALmesh->collect_garbage();
		//std::shared_ptr<Mesh> remeshedMesh = FormatTool::CGALMeshToMesh(m_CGALmesh, glm::mat4(1.0f));
		//remeshedMesh->SetName("Remeshed Result");

		// Show remesh result.
		m_Scene->AddMesh(remeshedMesh);
		remeshedMesh->GetPsMesh()->setEnabled(false);

		std::tie(m_GCTmesh, m_GCTgeo) = FormatTool::MeshToGCTMesh(remeshedMesh);

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

	glm::vec3 GeodesicTool::TracePath(Face_location face_loc, const glm::vec3& forward, float distance)
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
		std::cout << "ok" << std::endl;
		return parallelPaths;
	}

} // namespace GemCraft