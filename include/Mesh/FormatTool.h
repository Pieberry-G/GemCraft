#pragma once

#include "Mesh/Mesh.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel				Kernel;
typedef Kernel::Point_3													CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>									CGALMesh;

namespace GemCraft {
namespace FormatTool {

	std::shared_ptr<CGALMesh> MeshToCGALMesh(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform);
	std::shared_ptr<Mesh> CGALMeshToMesh(const std::shared_ptr<CGALMesh>& cgalmesh, const glm::mat4& transform);

} // namespace FormatTool
} // namespace GemCraft