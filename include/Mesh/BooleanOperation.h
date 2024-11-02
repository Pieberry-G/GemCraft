#pragma once

#include "Mesh/Mesh.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/convex_hull_3.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel				CGALKernel;
typedef CGALKernel::Point_3												CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>									CGALMesh;

typedef boost::graph_traits<CGALMesh>::halfedge_descriptor      halfedge_descriptor;
typedef boost::graph_traits<CGALMesh>::edge_descriptor          edge_descriptor;
typedef boost::graph_traits<CGALMesh>::face_descriptor          face_descriptor;

namespace CGALpmp = CGAL::Polygon_mesh_processing;
namespace CGALparams = CGAL::parameters;

namespace GemCraft {

	class BooleanOperation
	{
	public:
		std::shared_ptr<Mesh> DifferenceOperation(std::shared_ptr<Mesh> mainMesh, std::vector<std::shared_ptr<Mesh>> meshedToSubstract);
	private:
		std::shared_ptr<CGALMesh> ConstructContexHull(std::shared_ptr<CGALMesh> mesh);
	};

} // namespace GemCraft