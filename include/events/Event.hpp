#pragma once
#include "nlohmann/json.hpp"

enum EventType : uint32_t
{
	CLIENT_HID_EVENT,
	SERVER_SCENE_STATE_UPDATE_EVENT,
};

enum Action : uint32_t
{
	NODE_ROTATION_PRESERVE_Y_AS_UP,
	MOUSE_HOVER,
};

struct ClientHidEvent
{
	EventType event_type{ EventType::CLIENT_HID_EVENT };
	std::vector<Action> actions{ 0 };  // button inputs as a 4 byte array key/button mapping will be configurable
	double look_dx{ 0. };  // cursor x movement or right joystick
	double look_dy{ 0. };  // cursor y movement or right joystick
	double mouse_x{ 0 }; // cursor x coordinate in screen space
	double mouse_y{ 0 }; // cursor y coordinate in screen space

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ClientHidEvent, event_type, actions, look_dx, look_dy)
};
