#pragma once

#include "EventSystem/Input.h"
#include "EventSystem/KeyEvent.h"

namespace GemCraft {

	class MainMenu
	{
	public:
		MainMenu();

		void DrawMainMenu();
		std::function<void()> GetDrawUIFunction() { return std::bind(&MainMenu::DrawMainMenu, this); }

		bool OnKeyReleased(KeyReleasedEvent& e);

		void NewProject();
		void ExportMesh();
	};

} // namespace GemCraft