#pragma once

#include "Mesh/Mesh.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Shape_detection/Region_growing/Region_growing.h>
#include <CGAL/Shape_detection/Region_growing/Polygon_mesh.h>
#include <CGAL/mesh_segmentation.h>
#include <CGAL/property_map.h>
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel		Kernel;
typedef Kernel::Point_3											CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>							CGALMesh;

typedef boost::graph_traits<CGALMesh>::vertex_descriptor        vertex_descriptor;
typedef boost::graph_traits<CGALMesh>::face_descriptor          face_descriptor;
typedef boost::graph_traits<CGALMesh>::halfedge_descriptor		halfedge_descriptor;

using Neighbor_query = CGAL::Shape_detection::Polygon_mesh::One_ring_neighbor_query<CGALMesh>;

namespace CGALpmp = CGAL::Polygon_mesh_processing;

namespace GemCraft {

	class Scene;
	class SurfaceCleanerTool
	{
	public:
		SurfaceCleanerTool(Scene* scene)
			: m_Scene(scene) {}

		void Clean();

		void CleanSurface();
		void ShowResult();

	private:
		void RemoveProngs();
		void FillHoles();

	private:
		std::shared_ptr<Mesh> m_OriginalMesh;
		std::shared_ptr<Mesh> m_HollowedMesh;
		std::shared_ptr<Mesh> m_PatchedMesh;

	private:
		Scene* m_Scene;
	};

} // namespace GemCraft