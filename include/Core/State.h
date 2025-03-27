#pragma once

#include "Mesh/NurbsFitting.h"
#include "Mesh/MeshDeformation.h"

namespace GemCraft {
namespace State {

	extern std::unordered_map<polyscope::PointCloud*, NurbsFitting*> controlPointToNurbs;
	extern std::unordered_map<polyscope::PointCloud*, MeshDeformation*> controlPointToDeformation;

} // namespace State
} // namespace GemCraft