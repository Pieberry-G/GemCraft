#pragma once

#include "Mesh/Mesh.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits_3.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>
#include <CGAL/Surface_mesh_shortest_path.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel				Kernel;
typedef Kernel::Point_3													CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>									CGALMesh;

typedef boost::graph_traits<CGALMesh>::face_descriptor			        face_descriptor;

typedef CGAL::Surface_mesh_shortest_path_traits<Kernel, CGALMesh>		Traits;
typedef CGAL::Surface_mesh_shortest_path<Traits>						Surface_mesh_shortest_path;
typedef boost::graph_traits<CGALMesh>::face_iterator					face_iterator;

typedef CGAL::AABB_face_graph_triangle_primitive<CGALMesh>				AABB_face_graph_primitive;
typedef CGAL::AABB_traits_3<Kernel, AABB_face_graph_primitive>			AABB_face_graph_traits;
typedef CGAL::AABB_tree<AABB_face_graph_traits>                         AABB_tree;

namespace GemCraft {

	class GeodesicTool
	{
	public:
		GeodesicTool(std::shared_ptr<Mesh>& mesh);

		size_t QueryClosestFace(const glm::vec3& queryPoint);
		std::array<double, 3> QueryBarycentricCoords(const glm::vec3& queryPoint);
		glm::vec3 QueryClosestPoint(const glm::vec3& queryPoint);
		glm::vec3 CalculateNormal(const glm::vec3& position);

	private:
		std::shared_ptr<CGALMesh> m_CGALmesh;

		std::unique_ptr<Surface_mesh_shortest_path> m_ShortestPaths;
		AABB_tree m_Tree;
	};

} // namespace GemCraft