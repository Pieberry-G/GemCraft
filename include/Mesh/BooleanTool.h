#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/GemGroup.h"

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel					Kernel;
typedef CGAL::Exact_predicates_exact_constructions_kernel					ExactKernel;
typedef Kernel::Point_3														CGALPoint;
typedef Kernel::Vector_3													CGALVector;
typedef CGAL::Surface_mesh<CGALPoint>										CGALMesh;

typedef boost::graph_traits<CGALMesh>::vertex_descriptor					vertex_descriptor;
typedef boost::graph_traits<CGALMesh>::edge_descriptor						edge_descriptor;
typedef CGALMesh::Property_map<vertex_descriptor, ExactKernel::Point_3>		Exact_point_map;

namespace CGALpmp = CGAL::Polygon_mesh_processing;
namespace CGALparams = CGAL::parameters;

namespace GemCraft {

	struct Exact_vertex_point_map
	{
		// typedef for the property map
		typedef boost::property_traits<Exact_point_map>::value_type value_type;
		typedef boost::property_traits<Exact_point_map>::reference reference;
		typedef boost::property_traits<Exact_point_map>::key_type key_type;
		typedef boost::read_write_property_map_tag category;

		// exterior references
		Exact_point_map exact_point_map;
		CGALMesh* tm_ptr;

		// Converters
		CGAL::Cartesian_converter<Kernel, ExactKernel> to_exact;
		CGAL::Cartesian_converter<ExactKernel, Kernel> to_input;

		Exact_vertex_point_map()
			: tm_ptr(nullptr)
		{}

		Exact_vertex_point_map(const Exact_point_map& ep, CGALMesh& tm)
			: exact_point_map(ep)
			, tm_ptr(&tm)
		{
			for (CGALMesh::Vertex_index v : vertices(tm))
				exact_point_map[v] = to_exact(tm.point(v));
		}

		friend
		reference get(const Exact_vertex_point_map& map, key_type k)
		{
			CGAL_precondition(map.tm_ptr != nullptr);
			return map.exact_point_map[k];
		}

		friend
		void put(const Exact_vertex_point_map& map, key_type k, const ExactKernel::Point_3& p)
		{
			CGAL_precondition(map.tm_ptr != nullptr);
			map.exact_point_map[k] = p;
			// create the input point from the exact one
			map.tm_ptr->point(k) = map.to_input(p);
		}
	};

	class BooleanTool
	{
	public:
		std::shared_ptr<Mesh> DifferenceOperation(std::shared_ptr<Mesh>& ring, std::shared_ptr<Mesh>& meshToSubtract);
		std::shared_ptr<Mesh> DifferenceOperation(std::shared_ptr<Mesh>& ring, GemLine& gemLine);
		std::shared_ptr<Mesh> DifferenceOperation(std::shared_ptr<Mesh>& ring, GemGroup& gemGroup);

	private:
		std::shared_ptr<CGALMesh> ConstructContexHull(std::shared_ptr<CGALMesh> mesh);
	};

} // namespace GemCraft