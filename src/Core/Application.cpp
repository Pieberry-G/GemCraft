#include "Core/Application.h"
#include "Core/EntryPoint.h"

#include "Core/EventSystem.h"
#include "Core/Input.h"

#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"

namespace GemCraft {

	Application* Application::s_Instance = nullptr;

	Application::Application()
    {
		GC_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

        polyscope::init();
        polyscope::render::engine->setEventCallback(GC_BIND_EVENT_FN(Application::OnEvent));

        m_ResourceManager = ResourceManager::Get();
        m_ResourceManager->PreloadGems();
        m_ResourceManager->PreloadGemSettings();
        m_ResourceManager->PreloadMandrel();

        polyscope::state::edgeLengthScale = 0.3;

        m_Scene = std::make_shared<Scene>();
	}

	void Application::Run()
    {
        //const std::string gemPath = "../assets/Gems/RoundGem1.obj";
        //m_Scene->AddGem("Gem", gemPath, glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, 0.0f }));

        const std::string ringPath = "../assets/ring/ring0.stl";
        m_Scene->AddRing("Ring", ringPath);
        m_Scene->InitGeodesic();

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