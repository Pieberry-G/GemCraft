#include "Core/Application.h"
#include "Core/EntryPoint.h"

#include "EventSystem/Event.h"
#include "TinyRenderer/RenderTool.h"

#include "Tools/DatasetBuilder.h"

namespace GemCraft {

	Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        GC_CORE_WARN("GemCraft Application Launch!");

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

        Tools::DatasetBuilder builder;
        builder.BuildDataset();
    }

	void Application::Run()
    {
        const std::string filepath = "../assets/meshes/Gen/gen005/gen005.obj";
        GC_CORE_WARN("Loading ring file: {0}", filepath);
        m_Scene->AddRing("Ring", filepath);
        //m_Scene->InitGeodesic();

        m_Scene->AutoSelectRegion();

        // Get indices for element picking
        std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
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