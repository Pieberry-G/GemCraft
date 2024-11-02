#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace GemCraft {

	class Path
	{
	public:
		void AddPoint(const glm::vec3& point) { m_Points.push_back(point); }
		size_t Length() const { return m_Points.size(); }
		std::vector<glm::vec3> Points() const { return m_Points; }

	private:
		std::vector<glm::vec3> m_Points;
	};

} // namespace GemCraft