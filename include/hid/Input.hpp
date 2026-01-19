#include "events/Event.hpp"
#include<deque>

namespace ENG {
	class Node;
}

struct WindowUserData {
	double cursor_x{ 0. };
	double cursor_y{ 0. };
	std::deque<ClientHidEvent> eventQueue;
	bool windowResized{ false };
};


struct GLFWwindow;

/*
* Blender style camera - rotation around y axis is global, rotation around x axis is local to active object.
* Reverses rotation around y-axis when camera is 'upside down'
*/
void node_rotation_follows_input_preserve_y_as_up(ENG::Node& activeNode, const double dx, const double dy);
void node_rotation_follows_input(ENG::Node& activeNode, const double dx, const double dy);
void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_movement_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void framebufferResizeCallback(GLFWwindow* window, int width, int height);
