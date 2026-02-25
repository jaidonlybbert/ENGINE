#include "scenes/SceneWorldInput.hpp"
#include "GLFW/glfw3.h"
#include "scene/Scene.hpp"
#include "logger/Logging.hpp"
#include "events/Event.hpp"
#include "hid/Input.hpp"

#include<iostream>
#include<functional>

void SceneWorldInput::set_callbacks()
{
	InputCallbacks inputCallbacks;
	inputCallbacks.framebufferResizeCallbacks.push_back(framebufferResizeCallback);
	inputCallbacks.keyCallbacks.push_back(key_callback);
	inputCallbacks.mouseButtonCallbacks.push_back(mouse_button_callback);
	inputCallbacks.mouseScrollCallbacks.push_back(mouse_scroll_callback);
	inputCallbacks.mouseMovementCallbacks.push_back(mouse_movement_callback);

	InputController::set_callbacks(std::move(inputCallbacks));
}


void SceneWorldInput::mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Print the scroll offsets
	ENG_LOG_TRACE("Scroll Offset - X: " << xoffset << " Y: " << yoffset << std::endl);

	static double dx, dy = 0.f;
	static const auto invert_x = true;
	static const auto invert_y = false;
	static const auto sensitivity = static_cast<double>(1.f);

	xoffset *= sensitivity;
	yoffset *= sensitivity;
	xoffset = invert_x ? -xoffset : xoffset;
	yoffset = invert_y ? -yoffset : yoffset;

	auto* windowUserData = static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));

	const auto& shift_state = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);

	ClientHidEvent hidEvent{};
	hidEvent.look_dx = xoffset;
	hidEvent.look_dy = yoffset;
	hidEvent.actions.push_back(Action::NODE_ROTATION_PRESERVE_Y_AS_UP);
	windowUserData->eventQueue.push_back(hidEvent);
}


void SceneWorldInput::mouse_movement_callback(GLFWwindow* window, double xpos, double ypos)
{
	static double dx, dy = 0.f;

	auto* windowUserData = static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
	const auto& middle_mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);

	if (middle_mouse_state == GLFW_PRESS)
	{
		ENG_LOG_TRACE("Middle mouse down" << std::endl);
		dx = xpos - windowUserData->cursor_x;
		dy = ypos - windowUserData->cursor_y;
		windowUserData->cursor_x = xpos;
		windowUserData->cursor_y = ypos;

		ClientHidEvent hidEvent{};
		hidEvent.look_dx = dx;
		hidEvent.look_dy = dy;
		hidEvent.actions.push_back(Action::NODE_ROTATION_PRESERVE_Y_AS_UP);
		windowUserData->eventQueue.push_back(hidEvent);
	}
}

void SceneWorldInput::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		ENG_LOG_TRACE("Toggle settings window visibility" << std::endl);
		auto* sceneState = static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
	}
	if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		ENG_LOG_TRACE("E key down" << std::endl);
		auto* windowUserData = static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
		auto dx = glm::angleAxis(glm::radians(3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		ENG_LOG_TRACE("R key down" << std::endl);
		auto* windowUserData = static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
	}
}

void SceneWorldInput::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	auto* windowUserData = static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));

	if (button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if (action == GLFW_PRESS) // && initial_press)
		{
			glfwGetCursorPos(window, &windowUserData->cursor_x, &windowUserData->cursor_y);
			ENG_LOG_TRACE("Middle mouse initial press" << std::endl);
		}
		else // action is GLFW_RELEASE
		{
			ENG_LOG_TRACE("Middle mouse released" << std::endl);
		}
	}
}

void SceneWorldInput::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto* userData = static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
	userData->windowResized = true;
}
