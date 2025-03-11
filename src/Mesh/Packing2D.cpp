#include "Mesh/Packing2D.h"

namespace GemCraft {

	static std::pair<glm::vec2, float> CalculateBoundingCircle(const std::vector<glm::vec2>& points);
	static bool IsInsideBoundary(const glm::vec2& point, float offsetRadius, const std::vector<glm::vec2>& boundary);
	static std::vector<glm::vec2> ClipPointsToBoundary(const std::vector<glm::vec2>& points, float offsetRadius, const std::vector<glm::vec2>& boundary);
	static size_t FindClosestEdge(const glm::vec2& point, const std::vector<glm::vec2>& boundary);
	static std::vector<glm::vec2> AdjustCircles(const std::vector<glm::vec2>& circles, float cellRadius, const std::vector<glm::vec2>& boundary, float shrinkLength);

	std::vector<glm::vec2> Packing2D::GenerateSquarePacking(float cellRadius, float gridRotation, const std::vector<glm::vec2>& boundary, float shrinkLength)
	{
		std::vector<glm::vec2> points;
		float dx = 2 * cellRadius;	// horizontal spacing
		float dy = 2 * cellRadius;	// vertical spacing
		float cosTheta = std::cos(gridRotation);
		float sinTheta = std::sin(gridRotation);

		auto [center, boundingRadius] = CalculateBoundingCircle(boundary);
		int gridStep = 2.0 * boundingRadius / cellRadius;
		for (int i = -gridStep; i <= gridStep; i++) {
			for (int j = -gridStep; j <= gridStep; j++) {
				glm::vec2 offset(i * dx, j * dy);
				glm::vec2 rotatedOffset;
				rotatedOffset.x = offset.x * cosTheta - offset.y * sinTheta;
				rotatedOffset.y = offset.x * sinTheta + offset.y * cosTheta;
				points.push_back({ center.x + rotatedOffset.x, center.y + rotatedOffset.y });
			}
		}

		std::vector<glm::vec2> clippedPoints = ClipPointsToBoundary(points, cellRadius + shrinkLength, boundary);
		return clippedPoints;
	}

	std::vector<glm::vec2> Packing2D::GenerateHexagonalPacking(float cellRadius, float gridRotation, const std::vector<glm::vec2>& boundary, float shrinkLength)
	{
		std::vector<glm::vec2> points;
		float dx = cellRadius * sqrt(3) * 2.0f;	// horizontal spacing
		float dy = cellRadius;					// vertical spacing
		float cosTheta = std::cos(gridRotation);
		float sinTheta = std::sin(gridRotation);

		auto [center, boundingRadius] = CalculateBoundingCircle(boundary);
		int gridStep = 2.0 * boundingRadius / cellRadius;
		for (int i = -gridStep; i <= gridStep; i++) {
			for (int j = -gridStep; j <= gridStep; j++) {
				glm::vec2 offset(i * dx, j * dy);
				if (j % 2 != 0) {
					offset.x += dx * 0.5f; // even row offset
				}
				glm::vec2 rotatedOffset;
				rotatedOffset.x = offset.x * cosTheta - offset.y * sinTheta;
				rotatedOffset.y = offset.x * sinTheta + offset.y * cosTheta;
				points.push_back({ center.x + rotatedOffset.x, center.y + rotatedOffset.y });
			}
		}

		std::vector<glm::vec2> clippedPoints = ClipPointsToBoundary(points, cellRadius + shrinkLength, boundary);
		return clippedPoints;
	}

	std::vector<glm::vec2> Packing2D::GenerateCompactPackingOld(float cellRadius, float gridRotation, const std::vector<glm::vec2>& boundary, float shrinkLength, float packingEdgeLoopDensity, float packingCenterDensity)
	{
		std::vector<glm::vec2> points;
		float dx = (1.0f / packingCenterDensity) * cellRadius * sqrt(3) * 2.0f;	// horizontal spacing
		float dy = (1.0f / packingCenterDensity) * cellRadius;					// vertical spacing
		float cosTheta = std::cos(gridRotation);
		float sinTheta = std::sin(gridRotation);

		auto [center, boundingRadius] = CalculateBoundingCircle(boundary);
		int gridStep = 2.0 * boundingRadius / ((1.0f / packingCenterDensity) * cellRadius);
		for (int i = -gridStep; i <= gridStep; i++) {
			for (int j = -gridStep; j <= gridStep; j++) {
				glm::vec2 offset(i * dx, j * dy);
				if (j % 2 != 0) {
					offset.x += dx * 0.5f; // even row offset
				}
				glm::vec2 rotatedOffset;
				rotatedOffset.x = offset.x * cosTheta - offset.y * sinTheta;
				rotatedOffset.y = offset.x * sinTheta + offset.y * cosTheta;
				points.push_back({ center.x + rotatedOffset.x, center.y + rotatedOffset.y });
			}
		}

		points = ClipPointsToBoundary(points, cellRadius + shrinkLength, boundary);
		points = AdjustCircles(points, cellRadius, boundary, shrinkLength);
		return points;
	}

	std::vector<glm::vec2> Packing2D::GenerateCompactPacking(float cellRadius, float gridRotation, const std::vector<glm::vec2>& boundary, float shrinkLength, float packingEdgeLoopDensity, float packingCenterDensity)
	{
		std::vector<glm::vec2> points;
		float dx = (1.0f / packingCenterDensity) * cellRadius * sqrt(3) * 2.0f;	// horizontal spacing
		float dy = (1.0f / packingCenterDensity) * cellRadius;					// vertical spacing
		float cosTheta = std::cos(gridRotation);
		float sinTheta = std::sin(gridRotation);

		auto [center, boundingRadius] = CalculateBoundingCircle(boundary);
		int gridStep = 2.0 * boundingRadius / ((1.0f / packingCenterDensity) * cellRadius);
		for (int i = -gridStep; i <= gridStep; i++) {
			for (int j = -gridStep; j <= gridStep; j++) {
				glm::vec2 offset(i * dx, j * dy);
				if (j % 2 != 0) {
					offset.x += dx * 0.5f; // even row offset
				}
				glm::vec2 rotatedOffset;
				rotatedOffset.x = offset.x * cosTheta - offset.y * sinTheta;
				rotatedOffset.y = offset.x * sinTheta + offset.y * cosTheta;
				points.push_back({ center.x + rotatedOffset.x, center.y + rotatedOffset.y });
			}
		}

		points = ClipPointsToBoundary(points, cellRadius + shrinkLength + cellRadius, boundary);

		float perimeter = 0.0f;
		for (size_t i = 0; i < boundary.size(); ++i) {
			glm::vec2 a = boundary[i];
			glm::vec2 b = boundary[(i + 1) % boundary.size()];
			perimeter += glm::length(b - a);
		}
		float spacing = 2 * cellRadius * (1.0f / packingEdgeLoopDensity);
		int numCircles = static_cast<int>(perimeter / spacing);

		for (int i = 0; i < numCircles; ++i) {
			float targetLength = (i * spacing) + cellRadius;
			float accumulatedLength = 0.0f;
			while (accumulatedLength < targetLength) {
				for (size_t j = 0; j < boundary.size(); ++j) {
					glm::vec2 a = boundary[j];
					glm::vec2 b = boundary[(j + 1) % boundary.size()];
					float segmentLength = glm::length(b - a);
					if (accumulatedLength + segmentLength >= targetLength) {
						float t = (targetLength - accumulatedLength) / segmentLength;
						glm::vec2 position = a + t * (b - a);
						glm::vec2 tagent = glm::normalize(b - a);
						glm::vec2 normal = { tagent.y, -tagent.x };
						position = position + (cellRadius + shrinkLength) * normal;
						accumulatedLength += segmentLength;
						points.push_back(position);
						break;
					}
					else {
						accumulatedLength += segmentLength;
					}
				}
			}
		}

		points = AdjustCircles(points, cellRadius, boundary, shrinkLength);
		return points;
	}

	static std::pair<glm::vec2, float> CalculateBoundingCircle(const std::vector<glm::vec2>& points)
	{
		float min_x = std::numeric_limits<float>::max();
		float max_x = std::numeric_limits<float>::min();
		float min_y = std::numeric_limits<float>::max();
		float max_y = std::numeric_limits<float>::min();
		for (const auto& p : points) {
			min_x = std::min(min_x, p.x);
			max_x = std::max(max_x, p.x);
			min_y = std::min(min_y, p.y);
			max_y = std::max(max_y, p.y);
		}
		glm::vec2 center((min_x + max_x) / 2.0, (min_y + max_y) / 2.0);
		float radius = 0.0;
		for (const auto& p : points) {
			float distance = glm::length(p - center);
			radius = std::max(radius, distance);
		}
		return { center, radius };
	}

	static bool IsInsideBoundary(const glm::vec2& point, float offsetRadius, const std::vector<glm::vec2>& boundary)
	{
		std::vector<CGALPoint2> cgalBoundary;
		cgalBoundary.reserve(boundary.size());
		for (auto& point : boundary) {
			cgalBoundary.push_back({ point.x, point.y });
		}
		CGAL::Polygon_2<Kernel> polygon(cgalBoundary.begin(), cgalBoundary.end());

		int nSamples = 100;
		for (int i = 0; i < nSamples; i++) {
			float angle = 2.0f * glm::pi<float>() * i / nSamples;
			CGALPoint2 pointOnCircle(point.x + offsetRadius * std::cos(angle), point.y + offsetRadius * std::sin(angle));
			if (polygon.bounded_side(pointOnCircle) != CGAL::ON_BOUNDED_SIDE) {
				return false;
			}
		}
		return true;
	}

	static std::vector<glm::vec2> ClipPointsToBoundary(const std::vector<glm::vec2>& points, float offsetRadius, const std::vector<glm::vec2>& boundary)
	{
		std::vector<glm::vec2> clippedPoints;
		for (const auto& point : points) {
			if (IsInsideBoundary(point, offsetRadius, boundary)) {
				clippedPoints.push_back(point);
			}
		}
		return clippedPoints;
	}

	static size_t FindClosestEdge(const glm::vec2& point, const std::vector<glm::vec2>& boundary)
	{
		size_t closestEdge = 0;
		float minDistance = std::numeric_limits<float>::max();
		for (size_t i = 0; i < boundary.size(); ++i) {
			glm::vec2 a = boundary[i];
			glm::vec2 b = boundary[(i + 1) % boundary.size()];
			glm::vec2 ab = b - a;
			glm::vec2 ap = point - a;
			float t = glm::dot(ap, ab) / glm::dot(ab, ab);
			t = glm::clamp(t, 0.0f, 1.0f);
			glm::vec2 closestPoint = a + t * ab;
			float distance = glm::length(point - closestPoint);
			if (distance < minDistance) {
				minDistance = distance;
				closestEdge = i;
			}
		}
		return closestEdge;
	}

	static std::vector<glm::vec2> AdjustCircles(const std::vector<glm::vec2>& originCircles, float cellRadius, const std::vector<glm::vec2>& boundary, float shrinkLength)
	{
		std::vector<glm::vec2> circles = originCircles;
		float stepSize = cellRadius / 20.0f;
		for (int iter = 0; iter < 1000; iter++) {
			for (size_t i = 0; i < circles.size(); ++i) {
				glm::vec2 force(0.0f);
				for (size_t j = 0; j < circles.size(); ++j) {
					if (i != j) {
						glm::vec2 diff = circles[i] - circles[j];
						float distance = glm::length(diff);
						if (distance < 2.1f * cellRadius) {
							force += diff / distance * (2.1f * cellRadius - distance);
						}
					}
				}
				glm::vec2 newPosition = circles[i] + stepSize * force;
				if (!IsInsideBoundary(newPosition, cellRadius + shrinkLength, boundary)) {
					size_t closestEdge = FindClosestEdge(newPosition, boundary);
					glm::vec2 a = boundary[closestEdge];
					glm::vec2 b = boundary[(closestEdge + 1) % boundary.size()];
					glm::vec2 ab = b - a;
					glm::vec2 ap = newPosition - a;
					float t = glm::dot(ap, ab) / glm::dot(ab, ab);
					t = glm::clamp(t, 0.0f, 1.0f);
					glm::vec2 closestPoint = a + t * ab;
					newPosition = closestPoint + (cellRadius + shrinkLength) * glm::normalize(newPosition - closestPoint);
				}
				circles[i] = newPosition;
			}
		}
		std::vector<glm::vec2> result;
		for (auto& circle : circles) {
			if (IsInsideBoundary(circle, 0.9f * cellRadius + shrinkLength, boundary)) {
				result.push_back(circle);
			}
		}
		return result;
	}
}