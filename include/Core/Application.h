#pragma once

#include "Core/EventSystem.h"
#include "polyscope/polyscope.h"

#include "ResourceManager.h"
#include "Scene.h"

namespace GemCraft {

	class Application
	{
	public:
		Application();

		void Run();

		void OnEvent(Event& e);
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnKeyReleased(KeyReleasedEvent& e);
		bool OnAppRender(AppRenderEvent& e);

		std::shared_ptr<Scene> GetScene() { return m_Scene; }

		static Application* Get() { return s_Instance; }
	private:
		bool m_Running = true;

		ResourceManager* m_ResourceManager;
		std::shared_ptr<Scene> m_Scene;
	private:
		static Application* s_Instance;
	};

	Application* CreateApplication()
	{
		return new Application();
	}

} // namespace GemCraft