#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/MeshSubset.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/smooth_shape.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel		Kernel;
typedef Kernel::Point_3											CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>							CGALMesh;

typedef boost::graph_traits<CGALMesh>::vertex_descriptor        vertex_descriptor;
typedef boost::graph_traits<CGALMesh>::face_descriptor          face_descriptor;
typedef boost::graph_traits<CGALMesh>::edge_descriptor			edge_descriptor;
typedef boost::graph_traits<CGALMesh>::halfedge_descriptor		halfedge_descriptor;

namespace CGALpmp = CGAL::Polygon_mesh_processing;

namespace GemCraft {

	class Scene;
	class GeometryTool
	{
	public:
		GeometryTool(Scene* scene)
			: m_Scene(scene) {}

		void RepairGeometry();
		void ShowResult();

	private:
		void RemoveSelectedRegion();
		void FillHoles();

		void ShapeSmoothing();

	private:
		// Intermediate result
		std::shared_ptr<Mesh> m_HollowedMesh;
		std::shared_ptr<Mesh> m_PatchedMesh;
		std::shared_ptr<Mesh> m_SmoothedMesh;

		std::vector<float> m_Distance;

	private:
		Scene* m_Scene;
	};

} // namespace GemCraft