#pragma once

#include "Panels/MainMenu.h"
#include "Core/ResourceManager.h"
#include "Core/Scene.h"

#include "EventSystem/ApplicationEvent.h"
#include "EventSystem/KeyEvent.h"

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

		std::unique_ptr<Scene>& GetScene() { return m_Scene; }

		static Application* Get() { return s_Instance; }

	private:
		MainMenu m_MainMenu;
		ResourceManager* m_ResourceManager;

		std::unique_ptr<Scene> m_Scene;

	private:
		static Application* s_Instance;
	};

} // namespace GemCraft