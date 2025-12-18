#include "GLFW/glfw3.h"
#include "scene/Scene.hpp"
#include "logger/Logging.hpp"
#include<iostream>


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

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Print the scroll offsets
	printf("Scroll Offset - X: %.2f, Y: %.2f\n", xoffset, yoffset);

	static double dx, dy = 0.f;
	static const auto invert_x = true;
	static const auto invert_y = false;
	static const auto sensitivity = static_cast<double>(1.f);

	xoffset *= sensitivity;
	yoffset *= sensitivity;
	xoffset = invert_x ? -xoffset : xoffset;
	yoffset = invert_y ? -yoffset : yoffset;

	auto* sceneState = static_cast<ENG::SceneState*>(glfwGetWindowUserPointer(window));

	const auto& shift_state = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);

	if (sceneState->activeNodeIdx >= sceneState->graph.nodes.size())
	{
		ENG_LOG_ERROR("Active node idx is invalid!" << std::endl);
		return;
	}
	auto& activeNode = sceneState->graph.nodes.at(sceneState->activeNodeIdx);

	node_rotation_follows_input_preserve_y_as_up(activeNode, xoffset, yoffset);

	if (shift_state == GLFW_PRESS)
	{
		ENG_LOG_TRACE("Shift down" << std::endl);
		//auto& camera = sceneState->scene.cameras[sceneState->scene.nodes[sceneState->activeCameraNodeIdx].camera];
		// auto& fovy = camera.perspective.yfov;
		// fovy += 0.1 * yoffset;
		// if (static_cast<float>(fovy) < 0.1) fovy = 0.1; // Prevent zooming too far out
		// printf("FOV: %.2f\n", fovy);
	}
	else
	{
		// auto& camera_rotation = sceneState->scene.nodes[sceneState->activeCameraNodeIdx].rotation;
		// auto& camera_position = sceneState->scene.nodes[sceneState->activeCameraNodeIdx].translation;
		// assert(camera_position.size() == 3);
		// auto cam_quat = glm::quat(camera_rotation[3], camera_rotation[0], camera_rotation[1], camera_rotation[2]);

		// Default forward vector in world space
		glm::vec3 forward(0.0f, 0.0f, -1.0f);

		// Rotate the forward vector by the quaternion
		// glm::vec3 rotatedForward = cam_quat * forward;

		// Output the result
		// std::cout << "Forward vector: ("
		// 	<< rotatedForward.x << ", "
		// 	<< rotatedForward.y << ", "
		// 	<< rotatedForward.z << ")" << std::endl;

		// Move camera forward or backward
		// camera_position[0] += rotatedForward[0] * 0.1f;
		// camera_position[1] += rotatedForward[1] * 0.1f;
		// camera_position[2] += rotatedForward[2] * 0.1f;
		// std::cout << "Camera position: ("
		// 	<< camera_position[0] << ", "
		// 	<< camera_position[1] << ", "
		// 	<< camera_position[2] << ")" << std::endl;
	}
}


void mouse_movement_callback(GLFWwindow* window, double xpos, double ypos)
{
	static double dx, dy = 0.f;

	auto* sceneState = static_cast<ENG::SceneState*>(glfwGetWindowUserPointer(window));
	const auto& middle_mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);
	if (middle_mouse_state == GLFW_PRESS)
	{
		ENG_LOG_TRACE("Middle mouse down" << std::endl);
		dx = xpos - sceneState->cursor_x;
		dy = ypos - sceneState->cursor_y;
		sceneState->cursor_x = xpos;
		sceneState->cursor_y = ypos;

		ENG_LOG_TRACE("dx: " << dx << " dy: " << dx << std::endl);

		if (sceneState->activeNodeIdx >= sceneState->graph.nodes.size())
		{
			ENG_LOG_ERROR("Active node idx is invalid!" << std::endl);
			return;
		}
		auto& activeNode = sceneState->graph.nodes.at(sceneState->activeNodeIdx);

		node_rotation_follows_input(activeNode, dx, dy);
	}
	else 
	{
		//float x = (2.0f * mouseX) / screenWidth - 1.0f;
		//float y = 1.0f - (2.0f * mouseY) / screenHeight;
		//glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
		//glm::vec4 rayEye = glm::inverse(projMatrix) * rayClip;
		//rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
		//glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(viewMatrix) * rayEye));
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		ENG_LOG_TRACE("Toggle settings window visibility" << std::endl);
		auto* sceneState = static_cast<ENG::SceneState*>(glfwGetWindowUserPointer(window));
		// settings.showSettings = !settings.showSettings;
	}
	if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		ENG_LOG_TRACE("E key down" << std::endl);
		auto* sceneState = static_cast<ENG::SceneState*>(glfwGetWindowUserPointer(window));
		// auto& camera_rotation = sceneState->graph.nodes[sceneState->activeCameraNodeIdx].rotation;
		// auto cam_quat = glm::quat(camera_rotation[3], camera_rotation[0], camera_rotation[1], camera_rotation[2]);
		// rotate 3 degrees around x-axis when E is pressed
		auto dx = glm::angleAxis(glm::radians(3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// cam_quat = cam_quat * dx;
		// camera_rotation[0] = cam_quat.z;
		// camera_rotation[1] = cam_quat.x;
		// camera_rotation[2] = cam_quat.y;
		// camera_rotation[3] = cam_quat.w;
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		ENG_LOG_TRACE("R key down" << std::endl);
		auto* sceneState = static_cast<ENG::SceneState*>(glfwGetWindowUserPointer(window));
		// auto& test_rot = sceneState->test_model;
		// rotate 3 degrees around y-axis when E is pressed
		// auto dx = glm::angleAxis(glm::radians(3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// test_rot = glm::mat4_cast(dx) * test_rot;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	auto* sceneState = static_cast<ENG::SceneState*>(glfwGetWindowUserPointer(window));

	if (button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if (action == GLFW_PRESS) // && initial_press)
		{
			glfwGetCursorPos(window, &sceneState->cursor_x, &sceneState->cursor_y);
			ENG_LOG_TRACE("Middle mouse initial press" << std::endl);
		}
		else // action is GLFW_RELEASE
		{
			ENG_LOG_TRACE("Middle mouse released" << std::endl);
		}
	}
}
