#include "EventSystem/Input.h"

#include "polyscope/polyscope.h"

#include <GLFW/glfw3.h>

namespace GemCraft {

	bool Input::IsKeyPressed(KeyCode keycode)
	{
		auto window = (GLFWwindow*)polyscope::render::engine->getNativeWindow();
		auto state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed(MouseCode button)
	{
		auto window = (GLFWwindow*)polyscope::render::engine->getNativeWindow();
		auto state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		auto window = (GLFWwindow*)polyscope::render::engine->getNativeWindow();
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		return {(float)xpos, (float)ypos};
	}

	float Input::GetMouseX()
	{
		auto[x, y] = GetMousePosition();
		return x;
	}

	float Input::GetMouseY()
	{
		auto [x, y] = GetMousePosition();
		return y;
	}

} // namespace GemCraft