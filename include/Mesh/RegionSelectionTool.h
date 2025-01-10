#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/MeshSubset.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Shape_detection/Region_growing/Region_growing.h>
#include <CGAL/Shape_detection/Region_growing/Polygon_mesh.h>
#include "Mesh/RegionGrowingUtils.h"

namespace CGALpmp = CGAL::Polygon_mesh_processing;

typedef CGAL::Exact_predicates_inexact_constructions_kernel		Kernel;
typedef Kernel::Point_3											CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>							CGALMesh;
typedef Kernel::FT                                              FT;

using Neighbor_query = CGAL::Shape_detection::Polygon_mesh::One_ring_neighbor_query<CGALMesh>;
using Region_type = CGAL::Shape_detection::Polygon_mesh::Least_squares_plane_fit_region<Kernel, CGALMesh>;
using Sorting = CGAL::Shape_detection::Polygon_mesh::Least_squares_plane_fit_sorting<Kernel, CGALMesh, Neighbor_query>;
using Region_growing = CGAL::Shape_detection::Region_growing<Neighbor_query, Region_type>;

namespace GemCraft {

	class Scene;
	class RegionSelectionTool
	{
	public:
		RegionSelectionTool(Scene* scene)
			: m_Scene(scene) {}

		void AutoSelectRegion();
		void ShowResult();

	private:
		void RenderMultiviewImages();
		void SegmentMultiviewImages();
		void ApplyBackProjection();
		void SelectRegion();

	private:
		MeshSubset m_BackProjectionFaces;

	private:
		Scene* m_Scene;
	};

} // namespace GemCraft