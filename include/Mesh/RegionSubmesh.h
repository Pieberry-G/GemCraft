#pragma once

#include "Mesh/Mesh.h"
#include "Core/Scene.h"

#include "Mesh/Packing2D.h"
#include "Mesh/NurbsFitting.h"
#include "Mesh/MeshDeformation.h"
#include "Mesh/GeodesicTool.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/distance.h>
#include <CGAL/Polygon_mesh_processing/smooth_shape.h>
#include <CGAL/Heat_method_3/Surface_mesh_geodesic_distances_3.h>

#include <CGAL/Surface_mesh_parameterization/IO/File_off.h>
#include <CGAL/Surface_mesh_parameterization/ARAP_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/Error_code.h>
#include <CGAL/Surface_mesh_parameterization/parameterize.h>

#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/natural_neighbor_coordinates_2.h>


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

		void UpdateByNurbsFitting();

		std::shared_ptr<Mesh>& CreateMeshForBooleanHole(float holeDepth, float shrinkLength);

		std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> GenerateSquarePacking(float cellRadius, float gridRotation, float shrinkLength);
		std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> GenerateHexagonalPacking(float cellRadius, float gridRotation, float shrinkLength);
		std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> GenerateCompactPackingOld(float cellRadius, float gridRotation, float shrinkLength, float packingEdgeLoopDensity, float packingCenterDensity);
		std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> GenerateCompactPacking(float cellRadius, float gridRotation, float shrinkLength, float packingEdgeLoopDensity, float packingCenterDensity);

		void ShowResult(Scene* scene);
		void ShowBooleanMesh(Scene* scene);

	private:
		void BuildSubmesh(std::shared_ptr<Mesh>& mesh, MeshSubset selectedRegion);
		void InitGeodesicTool();
		void ParameterizeSubmesh();
		void CalculateGeodesicDistance();

		std::vector<glm::vec3> ExtractBooleanMeshBoundary(float shrinkLength);
		std::vector<glm::vec2> GetBoundary2D();
		std::vector<glm::vec3> Map2DPointsTo3D(std::vector<glm::vec2>& points);

	private:
		std::shared_ptr<Mesh> m_Submesh;
		std::unique_ptr<GeodesicTool> m_GeodesicTool;
		std::shared_ptr<Mesh> m_BooleanMesh;

		std::shared_ptr<NurbsFitting> m_NurbsFitting;
		std::shared_ptr<MeshDeformation> m_Deformation;

		// For every point in submesh
		std::vector<glm::vec2> m_UVCoords;
		std::vector<float> m_GeodesicDistances;
	};

} // namespace GemCraft