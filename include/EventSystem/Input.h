#pragma once

#include "EventSystem/KeyCodes.h"
#include "EventSystem/MouseCodes.h"

namespace GemCraft {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode keycode);

		static bool IsMouseButtonPressed(MouseCode button);
		static std::pair<float, float> GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};

} // namespace GemCraft