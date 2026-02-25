#include "GLFW/glfw3.h"
#include "scene/Scene.hpp"
#include "logger/Logging.hpp"
#include "events/Event.hpp"
#include "hid/Input.hpp"

#include<iostream>
#include<functional>

void InputController::mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	std::lock_guard<std::mutex> callbackLock(callbacksComplete);
	for (auto& callback : inputCallbacks.mouseScrollCallbacks)
	{
		callback(window, xoffset, yoffset);
	}
}

void InputController::mouse_movement_callback(GLFWwindow* window, double xpos, double ypos)
{
	std::lock_guard<std::mutex> callbackLock(callbacksComplete);
	for (auto& callback : inputCallbacks.mouseMovementCallbacks)
	{
		callback(window, xpos, ypos);
	}
}

void InputController::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	std::lock_guard<std::mutex> callbackLock(callbacksComplete);
	for (auto& callback : inputCallbacks.keyCallbacks)
	{
		callback(window, key, scancode, action, mods);
	}
}

void InputController::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	std::lock_guard<std::mutex> callbackLock(callbacksComplete);
	for (auto& callback : inputCallbacks.mouseButtonCallbacks)
	{
		callback(window, button, action, mods);
	}
}

void InputController::framebufferResizeCallback(GLFWwindow* window, int width, int height) 
{
	std::lock_guard<std::mutex> callbackLock(callbacksComplete);
	for (auto& callback : inputCallbacks.framebufferResizeCallbacks)
	{
		callback(window, width, height);
	}
}

void InputController::set_callbacks(InputCallbacks&& newInputCallbacks)
{
	std::lock_guard<std::mutex> callbackLock(callbacksComplete);
	inputCallbacks = newInputCallbacks;
}

/*
* Blender style camera - rotation around y axis is global, rotation around x axis is local to active object.
* Reverses rotation around y-axis when camera is 'upside down'
*/
void node_rotation_follows_input_preserve_y_as_up(ENG::Node& activeNode, const double dx, const double dy)
{

	ENG_LOG_TRACE("dx: " << dx << " dy: " << dy << std::endl);
	constexpr auto sensitivity = 1.0f;

	const auto invert = (activeNode.rotation * glm::vec3(0.f, 1.f, 0.f)).y >= 0.f ? 1.f : -1.f;

	auto dx_quat = glm::angleAxis(glm::radians(static_cast<float>(dx) * sensitivity), glm::vec3(0.f, invert, 0.f));
	auto dy_quat = glm::angleAxis(glm::radians(static_cast<float>(dy) * sensitivity), glm::vec3(1.f, 0.f, 0.f));
	activeNode.rotation = dx_quat * activeNode.rotation * dy_quat;
}

void node_rotation_follows_input(ENG::Node& activeNode, const double dx, const double dy)
{

	ENG_LOG_TRACE("dx: " << dx << " dy: " << dy << std::endl);
	constexpr auto sensitivity = 1.0f;

	auto dx_quat = glm::angleAxis(glm::radians(static_cast<float>(dx) * sensitivity), glm::vec3(0.f, 1.f, 0.f));
	auto dy_quat = glm::angleAxis(glm::radians(static_cast<float>(dy) * sensitivity), glm::vec3(1.f, 0.f, 0.f));
	activeNode.rotation = activeNode.rotation * dx_quat * dy_quat;
}
