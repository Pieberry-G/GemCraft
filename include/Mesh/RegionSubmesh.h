#pragma once

#include "Mesh/Mesh.h"
#include "Core/Scene.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/smooth_shape.h>

#include <CGAL/Surface_mesh_parameterization/IO/File_off.h>
#include <CGAL/Surface_mesh_parameterization/ARAP_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/Error_code.h>
#include <CGAL/Surface_mesh_parameterization/parameterize.h>

#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/natural_neighbor_coordinates_2.h>

#include <CGAL/Heat_method_3/Surface_mesh_geodesic_distances_3.h>
#include <CGAL/Polygon_mesh_processing/distance.h>
#include <CGAL/Polygon_mesh_processing/smooth_shape.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel              Kernel;
typedef Kernel::Point_2											         CGALPoint2;
typedef Kernel::Point_3											         CGALPoint;
typedef Kernel::Vector_3										         CGALVector;
typedef CGAL::Surface_mesh<CGALPoint>							         CGALMesh;

typedef boost::graph_traits<CGALMesh>::vertex_descriptor		         vertex_descriptor;
typedef boost::graph_traits<CGALMesh>::edge_descriptor					 edge_descriptor;
typedef boost::graph_traits<CGALMesh>::halfedge_descriptor		         halfedge_descriptor;
typedef boost::graph_traits<CGALMesh>::face_descriptor			         face_descriptor;

typedef CGALMesh::Property_map<vertex_descriptor, CGALPoint2>	         UV_pmap;
typedef CGAL::Delaunay_triangulation_2<Kernel>					         Triangulation;
typedef CGAL::Polygon_2<Kernel>									         Polygon_2;

typedef CGALMesh::Property_map<vertex_descriptor, double>				 Vertex_distance_map;
typedef CGAL::Heat_method_3::Surface_mesh_geodesic_distances_3<CGALMesh> Heat_method;

namespace CGALpmp = CGAL::Polygon_mesh_processing;
namespace CGALsmp = CGAL::Surface_mesh_parameterization;

namespace GemCraft {

	class RegionSubmesh
	{
	public:
		RegionSubmesh(std::shared_ptr<Mesh>& mesh, MeshSubset selectedRegion);

		std::vector<glm::vec2> GetBoundary();
		std::vector<glm::vec3> Map2DPointsTo3D(std::vector<glm::vec2>& points);

		std::shared_ptr<Mesh>& CreateMeshForBooleanHole(float holeDepth, float shrinkLength);

		void ShowResult(Scene* scene);
		void ShowBooleanMesh(Scene* scene);

	private:
		void BuildSubmesh(std::shared_ptr<Mesh>& mesh, MeshSubset selectedRegion);
		void ParameterizeSubmesh();
		void CalculateGeodesicDistance();

		std::vector<glm::vec3> ExtractBooleanMeshBoundary(float shrinkLength);

	private:
		std::shared_ptr<Mesh> m_Submesh;
		std::shared_ptr<Mesh> m_BooleanMesh;

		// For every point in submesh
		std::vector<glm::vec2> m_UVCoords;
		std::vector<float> m_GeodesicDistances;
	};

} // namespace GemCraft