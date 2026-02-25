#include "GLFW/glfw3.h"
#include "scene/Scene.hpp"

struct SceneWorldInput
{
	static void set_callbacks();

	static void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void mouse_movement_callback(GLFWwindow* window, double xpos, double ypos);
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};