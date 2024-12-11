#include "Tools/DatasetBuilder.h"

#include "TinyRenderer/Model.h"
#include "TinyRenderer/Camera.h"

#include <filesystem>

namespace GemCraft {
namespace Tools {

	static const glm::vec4 Black{ 0.0f, 0.0f, 0.0f, 1.0f };
	static const glm::vec4 White{ 1.0f, 1.0f, 1.0f, 1.0f };

	void DatasetBuilder::BuildDataset()
	{
		std::filesystem::path sourceDir = "../assets/meshes/Dataset";

		if (!std::filesystem::exists(sourceDir) || !std::filesystem::is_directory(sourceDir)) {
			GC_CORE_ERROR("Directory does not exist or is not a directory: " + sourceDir.string());
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(sourceDir)) {
			if (entry.is_regular_file()) {
				std::filesystem::path filePath = entry.path();
				if (filePath.extension() == ".obj") {
					RenderGLTF(filePath.generic_string());
					GC_CORE_INFO("Processed .obj file: " + filePath.generic_string());
				}
			}
		}
	}

	void DatasetBuilder::RenderGLTF(const std::string& filepath)
	{
		TinyRenderer::Model model(filepath);
		TinyRenderer::Camera camera(model.GetRadius() * 2.5f);
		std::string outputDir = "../dataIO/ImageDataset/";
		std::filesystem::create_directories(outputDir + "Train/");
		std::filesystem::create_directories(outputDir + "GT/");

		// Create gltf shader program
		std::shared_ptr<polyscope::render::ShaderProgram> gltfShaderProgram = polyscope::render::engine->requestShader(
			"GLTF_VIEWER", std::vector<std::string>(), polyscope::render::ShaderReplacementDefaults::Process);

		// Bind frameBuffer
		polyscope::render::FrameBuffer* gltfViewerFb = polyscope::render::engine->gltfViewerFb.get();
		polyscope::render::engine->setDepthMode();
		polyscope::render::engine->setBlendMode();
		gltfViewerFb->resize(1024, 1024);
		gltfViewerFb->setViewport(0, 0, 1024, 1024);
		gltfViewerFb->clearColor = glm::vec3(1.0f);

		if (!gltfViewerFb->bindForRendering()) return;

		polyscope::render::engine->setBackfaceCull(true);

		const std::vector<std::array<float, 2>> angles = {
			{ 0.0f, 0.0f },
			{ 0.0f, 90.0f },
			{ 0.0f, 180.0f },
			{ 0.0f, 270.0f },
			{ 90.0f, 0.0f },
			{ -90.0f, 0.0f },
		};

		for (uint32_t i = 0; i < angles.size(); i++) {
			gltfViewerFb->clear();
			gltfViewerFb->clear(0, White);
			gltfViewerFb->clear(2, Black);
			camera.SetEulerAngles(angles[i][0], angles[i][1]);
			model.Draw(camera, gltfShaderProgram);
			model.SaveRenderResult(outputDir + "Train/" + std::filesystem::path(filepath).stem().string() + "_" + std::to_string(i) + ".jpg", 0);
			//model.SaveRenderResult(std::to_string(i) + "_Position.jpg", 1);
			model.SaveRenderResult(outputDir + "GT/" + std::filesystem::path(filepath).stem().string() + "_" + std::to_string(i) + ".png", 2);
		}

		polyscope::render::engine->setBackfaceCull(); // return to default setting
	}

} //namespace Tools
} // namespace GemCraft

