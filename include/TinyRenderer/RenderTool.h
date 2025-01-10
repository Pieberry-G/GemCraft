#pragma once

#include "TinyRenderer/Camera.h"
#include "TinyRenderer/Image.h"
#include "TinyRenderer/Model.h"
#include "TinyRenderer/Camera.h"

#include <polyscope/polyscope.h>

namespace GemCraft {
namespace TinyRenderer {

	struct RenderToolData
	{
		std::shared_ptr<polyscope::render::ShaderProgram> ShaderProgram;
		std::shared_ptr<Image> WhiteTexture;
		polyscope::render::FrameBuffer* TinyRendererFb;
	};

	class RenderTool
	{
	public:
		static void Init();
		static void Render(Model& model, Camera& camera);

		static void SaveRenderResult(const std::string& outFile, uint32_t location = 0);
		static void BackProjection(MeshSubset& subset, const std::string& maskFile);

	private:
		static void DrawModel(Model& model, Camera& camera, std::shared_ptr<polyscope::render::ShaderProgram> gltfShaderProgram);

	private:
		static std::unique_ptr<RenderToolData> s_Data;
	};

} // namespace TinyRenderer 
} // namespace GemCraft