#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/MeshSubset.h"

#include <CGAL/Polygon_mesh_processing/measure.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Shape_detection/Region_growing/Region_growing.h>
#include <CGAL/Shape_detection/Region_growing/Polygon_mesh.h>
#include "Mesh/RegionGrowingUtils.h"

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Min_sphere_of_points_d_traits_3.h>
#include <CGAL/Min_sphere_of_spheres_d.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel		Kernel;
typedef Kernel::Point_3											CGALPoint;
typedef Kernel::Vector_3										CGALVector;
typedef CGAL::Surface_mesh<CGALPoint>							CGALMesh;
typedef Kernel::FT                                              FT;

typedef CGAL::Simple_cartesian<double>							K;
typedef CGAL::Min_sphere_of_points_d_traits_3<K, double>		TraitsMS;
typedef CGAL::Min_sphere_of_spheres_d<TraitsMS>					MinSphere;
typedef K::Point_3												Point;

using Neighbor_query = CGAL::Shape_detection::Polygon_mesh::One_ring_neighbor_query<CGALMesh>;
using Region_type = CGAL::Shape_detection::Polygon_mesh::Least_squares_plane_fit_region<Kernel, CGALMesh>;
using Sorting = CGAL::Shape_detection::Polygon_mesh::Least_squares_plane_fit_sorting<Kernel, CGALMesh, Neighbor_query>;
using Region_growing = CGAL::Shape_detection::Region_growing<Neighbor_query, Region_type>;

namespace CGALpmp = CGAL::Polygon_mesh_processing;

namespace GemCraft {

	class Scene;
	class RegionSelectionTool
	{
	public:
		RegionSelectionTool(Scene* scene)
			: m_Scene(scene) {}

		void Clean();

		void AutoRecognizeGems();
		void AutoSelectRegion();
		void InteractiveFillRegion();
		void InteractiveSphereSelect();
		void ShowResult();

	private:
		void RenderMultiviewImages();
		void SegmentMultiviewImages();
		void ApplyBackProjection();
		void RecognizeGems();
		void SelectRegion();

	private:
		MeshSubset m_BackProjectionFaces;

	private:
		Scene* m_Scene;
	};

} // namespace GemCraft