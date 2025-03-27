#include "Core/State.h"

namespace GemCraft {
namespace State {

	std::unordered_map<polyscope::PointCloud*, NurbsFitting*> controlPointToNurbs;
	std::unordered_map<polyscope::PointCloud*, MeshDeformation*> controlPointToDeformation;

} // namespace State
} // namespace GemCraft