#include "TinyRenderer/Mesh.h"

namespace GemCraft {
namespace TinyRenderer {

	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, int materialIndex)
		: m_Vertices(vertices), m_Indices(indices), m_MaterialIndex(materialIndex) {}

} // namespace TinyRenderer
} // namespace GemCraft