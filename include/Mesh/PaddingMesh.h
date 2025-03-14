#pragma once

#include "Mesh/Mesh.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#include <CGAL/Polygon_mesh_processing/remesh.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel              Kernel;
typedef Kernel::Point_3											         CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>							         CGALMesh;

typedef boost::graph_traits<CGALMesh>::vertex_descriptor		         vertex_descriptor;
typedef boost::graph_traits<CGALMesh>::edge_descriptor					 edge_descriptor;
typedef boost::graph_traits<CGALMesh>::halfedge_descriptor		         halfedge_descriptor;
typedef boost::graph_traits<CGALMesh>::face_descriptor			         face_descriptor;

namespace CGALpmp = CGAL::Polygon_mesh_processing;

namespace GemCraft {

	class PaddingMesh
	{
	public:
		void CreatePaddingMesh(const glm::vec3& basePoint, float length, float width, float height, float radius);

	private:
		std::shared_ptr<Mesh> m_PaddingMesh;
	};

} // namespace GemCraft