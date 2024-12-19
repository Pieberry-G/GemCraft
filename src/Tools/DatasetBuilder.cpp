#include "Tools/DatasetBuilder.h"

#include "TinyRenderer/Model.h"
#include "TinyRenderer/Camera.h"
#include "TinyRenderer/RenderTool.h"

#include <filesystem>

namespace GemCraft {
namespace Tools {

	void DatasetBuilder::BuildDataset()
	{
		std::filesystem::path sourceDir = "../assets/meshes/Dataset";
		std::filesystem::path outpath = "../dataIO/ImageDataset";

		if (!std::filesystem::exists(sourceDir) || !std::filesystem::is_directory(sourceDir)) {
			GC_CORE_ERROR("Directory does not exist or is not a directory: " + sourceDir.string());
			return;
		}

		GC_CORE_WARN("Building Dataset...\n");
		for (const auto& entry : std::filesystem::directory_iterator(sourceDir)) {
			if (entry.is_regular_file()) {
				std::filesystem::path filePath = entry.path();
				if (filePath.extension() == ".obj") {

					std::filesystem::create_directory(outpath);
					std::filesystem::create_directory(outpath / "Render");
					std::filesystem::create_directory(outpath / "Mask");

					TinyRenderer::Model model(filePath.string());
					TinyRenderer::Camera camera(model.GetRadius() * 2.5f);
					const std::vector<std::array<float, 2>> angles = {
						{ 0.0f, 0.0f },
						{ 0.0f, 90.0f },
						{ 0.0f, 180.0f },
						{ 0.0f, 270.0f },
						{ 90.0f, 0.0f },
						{ -90.0f, 0.0f },
					};

					GC_CORE_TRACE("Processing .obj file: " + filePath.generic_string());
					GC_CORE_WARN("Rendering multiview images.");
					for (size_t i = 0; i < angles.size(); i++) {
						camera.SetEulerAngles(angles[i][0], angles[i][1]);
						TinyRenderer::RenderTool::Render(model, camera);
						TinyRenderer::RenderTool::SaveRenderResult((outpath / "Render" / filePath.stem()).string() + "_" + std::to_string(i) + ".jpg", 0);
						TinyRenderer::RenderTool::SaveRenderResult((outpath / "Mask" / filePath.stem()).string() + "_" + std::to_string(i) + ".png", 1);
					}
					GC_CORE_INFO("Rendering completed!\n");
				}
			}
		}
		GC_CORE_INFO("Dataset construction completed!\n");
	}

} //namespace Tools
} // namespace GemCraft

