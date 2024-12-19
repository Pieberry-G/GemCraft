#include "Mesh/PreprocessTool.h"

#include "Core/Scene.h"
#include "TinyRenderer/RenderTool.h"

#include <filesystem>

namespace GemCraft {

    static const std::vector<std::array<float, 2>> s_CameraAngles = {
            { 0.0f, 0.0f },
            { 0.0f, 90.0f },
            { 0.0f, 180.0f },
            { 0.0f, 270.0f },
            { 90.0f, 0.0f },
            { -90.0f, 0.0f },
    };

    void PreprocessTool::PreprocessRing()
    {
        GC_CORE_WARN("Begin to implement ring preprocess:");
        // Render multiview images
        RenderMultiviewImages();

        // Segment
        Segment();

        // Inverse projection
        InverseProjection();

        // Visualize
        m_Scene->ShowRingSelected();
        GC_CORE_INFO("Ring preprocess completed!");
    }

    void PreprocessTool::RenderMultiviewImages()
    {
        const std::string filepath = m_Scene->GetRing()->GetFilepath();

        // Render multiview images.
        std::filesystem::path renderOutpath = "../dataIO/InputImages";
        std::filesystem::remove_all(renderOutpath);
        std::filesystem::create_directories(renderOutpath);

        TinyRenderer::Model model(filepath);
        TinyRenderer::Camera camera(model.GetRadius() * 2.5f);

        GC_CORE_WARN("Rendering multiview images.");
        for (size_t i = 0; i < s_CameraAngles.size(); i++) {
            camera.SetEulerAngles(s_CameraAngles[i][0], s_CameraAngles[i][1]);
            TinyRenderer::RenderTool::Render(model, camera);
            TinyRenderer::RenderTool::SaveRenderResult((renderOutpath / std::filesystem::path(filepath).stem()).string() + "_" + std::to_string(i) + ".jpg", 0);
        }
        GC_CORE_INFO("Rendering completed!");
    }

	void PreprocessTool::Segment()
	{
		std::filesystem::create_directories("../dataIO/InputImages");
		std::filesystem::remove_all("../dataIO/OutputMasks");
		std::filesystem::create_directories("../dataIO/OutputMasks");

		GC_CORE_WARN("Performing segmentation prediction with SAM-Adapter.");
		GC_CORE_TRACE("Waiting...");
		system("..\\deps\\sam-adapter\\SegmentInfer.bat");
		GC_CORE_INFO("Segmentation prediction completed!");
	}

    void PreprocessTool::InverseProjection()
    {
        TinyRenderer::Model model(m_Scene->GetRing()->GetVertices(), m_Scene->GetRing()->GetFaces());
        TinyRenderer::Camera camera(model.GetRadius() * 2.5f);

        GC_CORE_WARN("Performing inverse projection.");
        for (size_t i = 0; i < s_CameraAngles.size(); i++) {
            camera.SetEulerAngles(s_CameraAngles[i][0], s_CameraAngles[i][1]);
            TinyRenderer::RenderTool::Render(model, camera);
            TinyRenderer::RenderTool::BackProjection("../dataIO/OutputMasks/" + std::to_string(i) + ".png");
        }
        GC_CORE_INFO("Inverse projection completed!");
    }

} // namespace GemCraft