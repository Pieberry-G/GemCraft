#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel     Kernel;
typedef Kernel::Point_2											CGALPoint2;

namespace GemCraft {

	class Packing2D
	{
	public:
		std::vector<glm::vec2> GenerateSquarePacking(float cellRadius, float gridRotation, const std::vector<glm::vec2>& boundary2D, float shrinkLength);
		std::vector<glm::vec2> GenerateHexagonalPacking(float cellRadius, float gridRotation, const std::vector<glm::vec2>& boundary2D, float shrinkLength);
		std::vector<glm::vec2> GenerateCompactPackingOld(float cellRadius, float gridRotation, const std::vector<glm::vec2>& boundary2D, float shrinkLength, float packingEdgeLoopDensity, float packingCenterDensity);
		std::vector<glm::vec2> GenerateCompactPacking(float cellRadius, float gridRotation, const std::vector<glm::vec2>& boundary2D, float shrinkLength, float packingEdgeLoopDensity, float packingCenterDensity);
	};

} // namespace GemCraft