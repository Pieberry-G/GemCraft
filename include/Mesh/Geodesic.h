#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/Path.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Surface_mesh_shortest_path.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>
#include <CGAL/AABB_traits_3.h>
#include <CGAL/AABB_tree.h>

#include <geometrycentral/surface/meshio.h>
#include <geometrycentral/surface/surface_point.h>
#include <geometrycentral/surface/trace_geodesic.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel				CGALKernel;
typedef CGALKernel::Point_3												CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>									CGALMesh;

typedef CGAL::Surface_mesh_shortest_path_traits<CGALKernel, CGALMesh>	Traits;
typedef CGAL::Surface_mesh_shortest_path<Traits>						Surface_mesh_shortest_path;
typedef boost::graph_traits<CGALMesh>									Graph_traits;
typedef Graph_traits::face_iterator										face_iterator;

typedef typename Surface_mesh_shortest_path::Face_location              Face_location;
typedef CGAL::AABB_face_graph_triangle_primitive<CGALMesh>				AABB_face_graph_primitive;
typedef CGAL::AABB_traits_3<CGALKernel, AABB_face_graph_primitive>		AABB_face_graph_traits;
typedef CGAL::AABB_tree<AABB_face_graph_traits>                         AABB_tree;

namespace GCT = geometrycentral;
namespace GCTsf = geometrycentral::surface;

namespace GemCraft {

	class Geodesic
	{
	public:
		Geodesic(std::shared_ptr<Mesh>& mesh);

		glm::vec3 CalculateNormal(const glm::vec3& position);

		Path ConstructGeodesicPath();
		std::vector<Path> CalculateParallelPaths(const Path& geodesicPath, int numberOfPaths, float pathSpacing);
	private:
		glm::vec3 TracePath(Face_location loc, const glm::vec3& forward, float distance);

	private:
		std::shared_ptr<Mesh> m_Mesh;

		std::shared_ptr<CGALMesh> m_CGALmesh;
		std::unique_ptr<GCTsf::ManifoldSurfaceMesh> m_GCTmesh;
		std::unique_ptr<GCTsf::VertexPositionGeometry> m_GCTgeo;

		std::unique_ptr<Surface_mesh_shortest_path> m_ShortestPaths;
		AABB_tree m_Tree;
	};

} // namespace GemCraft