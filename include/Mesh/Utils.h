#pragma once

#include "Mesh/Mesh.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#include <geometrycentral/surface/meshio.h>
#include <geometrycentral/surface/surface_point.h>
#include <geometrycentral/surface/surface_mesh_factories.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel				Kernel;
typedef Kernel::Point_3													CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>									CGALMesh;

namespace GCT = geometrycentral;
namespace GCTsf = geometrycentral::surface;

namespace GemCraft {

	namespace Utils {
		std::shared_ptr<CGALMesh> MeshToCGALMesh(std::shared_ptr<Mesh> mesh, const glm::mat4& transform);
		std::shared_ptr<Mesh> CGALMeshToMesh(std::shared_ptr<CGALMesh> cgalmesh, const glm::mat4& transform);
		std::tuple<std::unique_ptr<GCTsf::ManifoldSurfaceMesh>, std::unique_ptr<GCTsf::VertexPositionGeometry>>	MeshToGCTMesh(std::shared_ptr<Mesh> mesh);

	} // namespace Utils

} // namespace GemCraft