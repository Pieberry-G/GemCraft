#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/MeshSubset.h"
#include "Mesh/NurbsFitting.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/smooth_shape.h>
#include <CGAL/Shape_detection/Region_growing/Polygon_mesh.h>

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/stitch_borders.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel		Kernel;
typedef Kernel::Point_3											CGALPoint;
typedef Kernel::Vector_3										CGALVector;
typedef CGAL::Surface_mesh<CGALPoint>							CGALMesh;

typedef boost::graph_traits<CGALMesh>::vertex_descriptor        vertex_descriptor;
typedef boost::graph_traits<CGALMesh>::face_descriptor          face_descriptor;
typedef boost::graph_traits<CGALMesh>::edge_descriptor			edge_descriptor;
typedef boost::graph_traits<CGALMesh>::halfedge_descriptor		halfedge_descriptor;

using Neighbor_query = CGAL::Shape_detection::Polygon_mesh::One_ring_neighbor_query<CGALMesh>;

namespace CGALpmp = CGAL::Polygon_mesh_processing;

namespace GemCraft {

	class Scene;
	class GeometryTool
	{
	public:
		GeometryTool(Scene* scene)
			: m_Scene(scene) {}

		void Clean();

		void RepairGeometry();
		void ShowResult();

		void RemoveSelectedRegion();
	private:
		void FillHoles();

	private:
		// Intermediate result
		std::shared_ptr<Mesh> m_HollowedMesh;
		std::shared_ptr<Mesh> m_PatchedMesh;

		std::shared_ptr<NurbsFitting> m_NurbsFitting;

	private:
		Scene* m_Scene;
	};

} // namespace GemCraft