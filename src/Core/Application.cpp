#include "Core/Application.h"
#include "Core/EntryPoint.h"

#include "EventSystem/Event.h"
#include "Tools/DatasetBuilder.h"

#include "TinyRenderer/RenderTool.h"
#include "TinyRenderer/SegmentTool.h"

namespace GemCraft {

	Application* Application::s_Instance = nullptr;

	Application::Application()
    {
		GC_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

        polyscope::init();
        polyscope::render::engine->setEventCallback(GC_BIND_EVENT_FN(Application::OnEvent));

        TinyRenderer::RenderTool::Init();

        m_ResourceManager = ResourceManager::Get();
        m_ResourceManager->PreloadGems();
        m_ResourceManager->PreloadGemSettings();
        m_ResourceManager->PreloadMandrel();

        polyscope::state::edgeLengthScale = 0.3;

        m_Scene = std::make_shared<Scene>();

        //Tools::DatasetBuilder builder;
        //builder.BuildDataset();

        GC_CORE_WARN("GemCraft Application Launch!\n");

        //std::filesystem::path filePath = "../assets/meshes/Ring/ring0.stl";
        ////std::filesystem::path filePath = "../assets/meshes/Gen/gen_001.obj";
        ////std::filesystem::path filePath = "../assets/meshes/Ring/demo061.obj";
        //std::filesystem::path renderOutpath = "../dataIO/InputImages";
        //TinyRenderer::Model model(filePath.string());
        //TinyRenderer::Camera camera(model.GetRadius() * 2.5f);
        //const std::vector<std::array<float, 2>> angles = {
        //    { 0.0f, 0.0f },
        //    { 0.0f, 90.0f },
        //    { 0.0f, 180.0f },
        //    { 0.0f, 270.0f },
        //    { 90.0f, 0.0f },
        //    { -90.0f, 0.0f },
        //};

        //GC_CORE_WARN("Rendering multiview images.");
        //for (uint32_t i = 0; i < angles.size(); i++) {
        //    camera.SetEulerAngles(angles[i][0], angles[i][1]);
        //    TinyRenderer::RenderTool::Render(model, camera);
        //    TinyRenderer::RenderTool::SaveRenderResult((renderOutpath / filePath.stem()).string() + "_" + std::to_string(i) + ".jpg", 0);
        //}
        //GC_CORE_INFO("Rendering completed!\n");
        //TinyRenderer::RenderTool::Render(model, camera);
        ////SegmentTool::Segment();
        //TinyRenderer::RenderTool::BackProjection(model);
	}

	void Application::Run()
    {
        //const std::string gemPath = "../assets/Gems/RoundGem1.obj";
        //m_Scene->AddGem("Gem", gemPath, glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, 0.0f }));

        const std::string ringPath = "../assets/meshes/Ring/ring0.stl";
        //const std::string ringPath = "../assets/meshes/Gen/gen_001.obj";
        //const std::string ringPath = "../assets/meshes/Ring/demo061.obj";
        m_Scene->AddRing("Ring", ringPath);
        m_Scene->InitGeodesic();
        GC_CORE_TRACE("Loading model file: {0}\n", ringPath);

        //m_Scene->ShowRingSelected();

        // Get indices for element picking
        std::shared_ptr<Mesh> ring = m_Scene->GetRing();
        polyscope::state::facePickIndStart = ring->nVertices();
        polyscope::state::edgePickIndStart = polyscope::state::facePickIndStart + ring->nFaces();
        polyscope::state::halfedgePickIndStart = polyscope::state::edgePickIndStart + ring->nEdges();

        // Give control to the polyscope gui
        polyscope::show();
	}

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(GC_BIND_EVENT_FN(OnWindowClose));
        dispatcher.Dispatch<KeyReleasedEvent>(GC_BIND_EVENT_FN(Application::OnKeyReleased));
        dispatcher.Dispatch<AppRenderEvent>(GC_BIND_EVENT_FN(Application::OnAppRender));
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnKeyReleased(KeyReleasedEvent& e)
    {
        m_Scene->OnKeyReleased(e.GetKeyCode());
        return true;
    }

    bool Application::OnAppRender(AppRenderEvent& e)
    {
        m_Scene->OnRender(e.GetCommand());
        return true;
    }

} // namespace GemCraft