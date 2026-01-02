#include <stdio.h>
#include <thread>
#include <functional>

// Necessary definition for PMP header compilation
#ifndef M_PI
#define M_PI 3.1415926
#endif

#include "asio/post.hpp"
#include "asio/io_context.hpp"
#include "asio/co_spawn.hpp"
#include "asio/awaitable.hpp"
#include "asio/detached.hpp"

#ifdef _WIN32
#include "tracy/Tracy.hpp"
#endif

#include "nlohmann/json.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "renderer/vk/Renderer.hpp"
#include "renderer/vk_adapter/VkAdapter.hpp"
#include "logger/Logging.hpp"
#include "sockets/SocketSessionServer.h"
#include "scenes/SceneWorld.hpp"
#include "hid/Input.hpp"
#include "application/Application.hpp"
#include "guis/SceneGui.hpp"
#include "gui/Gui.hpp"
#include "scene/DFT.hpp"

#include "lua.hpp"

void stop(asio::io_context& io_context) {
	if (io_context.stopped()) {
		return;
	}

	io_context.stop();
}


lua_State* luaState;
void initLua() {
	luaState = luaL_newstate();
	if (luaState == nullptr) {
		throw std::runtime_error("Failed to initialize Lua");
	}
	luaL_openlibs(luaState);
	if (luaL_dostring(luaState, "x = 42; print('Hello from Lua 2!')") != LUA_OK) {
		throw std::runtime_error("Failed running test lua script from string");
	}
}

enum Action : uint32_t 
{
	PrimaryAction,
	SecondaryAction,
	PivotView
};

std::unordered_map<uint32_t, Action> MouseKeyMap{
	{ GLFW_MOUSE_BUTTON_LEFT, Action::PrimaryAction},
	{ GLFW_MOUSE_BUTTON_RIGHT, Action::SecondaryAction},
	{ GLFW_MOUSE_BUTTON_MIDDLE, Action::PivotView},
};

enum EventType : uint32_t
{
	CLIENT_HID_EVENT,
	SERVER_SCENE_STATE_UPDATE_EVENT
};

struct ClientHidEvent
{
	EventType event_type{ EventType::CLIENT_HID_EVENT };
	uint32_t actions{ 0 };  // button inputs as a 4 byte array key/button mapping will be configurable
	double look_dx{ 0. };  // cursor x movement or right joystick
	double look_dy{ 0. };  // cursor y movement or right joystick

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ClientHidEvent, event_type, actions, look_dx, look_dy)
};

inline void to_json(nlohmann::json& j, const glm::mat4& m) {
    j = nlohmann::json::array();
    for (int i = 0; i < 16; ++i)
        j.push_back(glm::value_ptr(m)[i]);
}

inline void from_json(const nlohmann::json& j, glm::mat4& m) {
    for (int i = 0; i < 16; ++i)
        glm::value_ptr(m)[i] = j.at(i).get<float>();
}

struct ServerSceneStateUpdateEvent
{
	EventType eventType{ EventType::SERVER_SCENE_STATE_UPDATE_EVENT };
	// std::vector<glm::mat4> modelMatrices;
	//glm::mat4 modelMatrices;
	//std::chrono::steady_clock::time_point serverTime;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ServerSceneStateUpdateEvent, eventType) //modelMatrices)
};

std::deque<nlohmann::json> clientSideEventSendQueue;
std::deque<nlohmann::json> clientSideEventReceiveQueue;
std::deque<nlohmann::json> serverSideEventSendQueue;
std::deque<nlohmann::json> serverSideEventReceiveQueue;

void sendEventClientToServer(nlohmann::json event_serialized) {
	clientSideEventSendQueue.push_back(event_serialized);
}

void sendEventServerToClient(nlohmann::json event_serialized) {
	serverSideEventSendQueue.push_back(event_serialized);
}

std::vector<glm::mat4> clientSidePredictCurrentModelMatrices(
	const std::vector<glm::mat4>& previousPrediction, 
	const std::vector<glm::mat4>& serverInput,
	const std::chrono::steady_clock::time_point previousTime,
	const std::chrono::steady_clock::time_point serverTime)
{
	const auto currentTime = std::chrono::steady_clock::now();

	// TODO: some kind of kalman filter - for now take server state
	return serverInput;
}

void serverSceneStateUpdateEventHandler(const ServerSceneStateUpdateEvent&& event_, SceneState& sceneState)
{
	const auto& modelMatrices = { glm::mat4() };// event_.modelMatrices;
	// const auto& server_time = std::move(event_.serverTime);
	const auto server_time = std::chrono::steady_clock::now();

	sceneState.modelMatrices = clientSidePredictCurrentModelMatrices(
		sceneState.modelMatrices, 
		modelMatrices,
		sceneState.previousPredictionTime,
		server_time);
}

void dispatchEventFromServer(nlohmann::json serializedEvent, SceneState& sceneState)
{
	EventType event_type = static_cast<EventType>(serializedEvent.at("eventType").get<uint32_t>());

	switch (event_type)
	{
		case EventType::SERVER_SCENE_STATE_UPDATE_EVENT:
		{
			serverSceneStateUpdateEventHandler(serializedEvent.get<ServerSceneStateUpdateEvent>(), sceneState);
			break;
		}
		default:
			ENG_LOG_ERROR("Client received event type " << event_type << " which is not a handled type." << std::endl);
			break;
	}
	return;
}

void dispatchEventFromClient(nlohmann::json serializedEvent)
{
	return;
}

void consumeServerEvent(SceneState& sceneState) {
	if (clientSideEventReceiveQueue.empty())
	{
		return;
	}
	nlohmann::json serializedEvent = clientSideEventReceiveQueue.front();
	clientSideEventReceiveQueue.pop_front();

	dispatchEventFromServer(serializedEvent, sceneState);
}

void consumeClientEvent() {
	if (serverSideEventReceiveQueue.empty())
	{
		return;
	}
	nlohmann::json serializedEvent = serverSideEventReceiveQueue.front();
	serverSideEventReceiveQueue.pop_front();

	dispatchEventFromClient(serializedEvent);
}

void initWindow(VkRenderer& app, SceneState& sceneState) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	app.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(app.window, &sceneState);
	glfwSetFramebufferSizeCallback(app.window, app.framebufferResizeCallback);
	glfwGetCursorPos(app.window, &sceneState.cursor_x, &sceneState.cursor_y);
	glfwSetScrollCallback(app.window, mouse_scroll_callback);
	glfwSetKeyCallback(app.window, key_callback);
	glfwSetMouseButtonCallback(app.window, mouse_button_callback);
	glfwSetCursorPosCallback(app.window, mouse_movement_callback);
}

void profilerMarkStart(const std::string& name)
{
#ifdef _WIN32
		FrameMarkStart("run_frame");
#endif
}

void profilerMarkStop(const std::string& name)
{
#ifdef _WIN32
		FrameMarkEnd("run_frame");
#endif
}

UniformBufferObject createUniformBufferObject(const SceneState& sceneState) 
{
	UniformBufferObject ubo{};

	auto& cameraNode = sceneState.graph.nodes.at(sceneState.activeCameraNodeIdx);
	auto* cameraPtr = get_active_camera(sceneState);

	// The global translation is stored in the last column
	const glm::vec3& cam_pos = glm::vec3(sceneState.modelMatrices.at(cameraNode.nodeId)[3]);
	const glm::vec3& up = sceneState.modelMatrices.at(cameraNode.nodeId) * glm::vec4(0., 1., 0., 0.);
	const glm::vec3& right = sceneState.modelMatrices.at(cameraNode.nodeId) * glm::vec4(1., 0., 0., 0.);

	ubo.view = glm::lookAt(cam_pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::normalize(up));
	//const auto& cam_rot = camera_node.rotation;
	//if (cam_rot.size() == 4)
	//{
	//	auto quat1 = glm::quat(cam_rot[3], cam_rot[0], cam_rot[1], cam_rot[2]);
	//	ubo.view = glm::mat4_cast(quat1);
	//	ubo.view[0][3] = glm_cam_pos.x;
	//	ubo.view[1][3] = glm_cam_pos.y;
	//	ubo.view[2][3] = glm_cam_pos.z;
	//}

	const auto fovy = cameraPtr->fovy;
	const auto aspect = cameraPtr->aspect;
	const auto znear = cameraPtr->znear;
	const auto zfar = cameraPtr->zfar;
	ubo.proj = glm::perspective(fovy, aspect, znear, zfar);
	ubo.proj[1][1] *= -1;
	ENG_LOG_TRACE("Projection matrix set" << std::endl);
	return ubo;
}

void updateModelMatrix(glm::mat4& modelMatrix, const ENG::Node& node)
{
	// Compute local transform from TRS data
	modelMatrix = glm::translate(glm::mat4(1.f), node.translation)
		* glm::mat4_cast(node.rotation)
		* glm::scale(glm::mat4(1.f), node.scale);
}

void updateModelMatrices(SceneState& sceneState)
{
	// Compute local transforms first from TRS data
	for (const auto& node : sceneState.graph.nodes)
	{
		assert(node.nodeId < sceneState.modelMatrices.size());
		updateModelMatrix(sceneState.modelMatrices.at(node.nodeId), node);
	}

	// Update to global transforms by depth-first traversal of the graph
	for (auto* node : DFTraversal(sceneState.graph.root))
	{
		if (node == sceneState.graph.root)
		{
			continue;  // Do not need to do any update on root node
		}

		// Global transform is computed as the transform of local transform by parents global transform
		// Depth-first traversal guarantees parent global transform is computed before child is visited
		assert(node != nullptr && node->nodeId < sceneState.modelMatrices.size());
		assert(node->parent != nullptr && node->parent->nodeId < sceneState.modelMatrices.size());
		sceneState.modelMatrices.at(node->nodeId) = sceneState.modelMatrices.at(node->parent->nodeId) * sceneState.modelMatrices.at(node->nodeId);
		ENG_LOG_TRACE("TRAVERSING NAME: " << node->name << std::endl);
	}
}

void gameLoop(VkRenderer& renderer, Gui& gui) {
	while(!glfwWindowShouldClose(renderer.window)) {
		profilerMarkStart("run_frame");

		glfwPollEvents();
		gui.drawGui();
		renderer.drawFrame();

		profilerMarkStop("run_frame");
	}

	vkDeviceWaitIdle(renderer.device);
}

int main() {
	
	try {
		ENG_LOG_INFO("Starting app" << std::endl);

		Application app;
		SceneState sceneState;
		Gui gui;
		SceneGui sceneGui;

		VkRenderer renderer{
			{
				[&renderer, &sceneState]() {initWindow(renderer, sceneState);},
				[&renderer]() {renderer.initVulkan();},
				[&renderer]() {renderer.initGui();},
				[]() {initLua();}
			}
		};

		gui.registerDrawCall([&sceneGui, &sceneState]() {sceneGui.drawGui(sceneState);});

		ENG_LOG_DEBUG(renderer);

		app.registerInitFunction("renderer.initializeScene()", [&renderer, &sceneState]() { 
			renderer.sceneReadyToRender = false;
			initializeWorldScene(renderer, sceneState);
			renderer.sceneReadyToRender = true;
			});
		app.registerCoroutineFunction("listener(tcp::acceptor)", []() {
			return listener(tcp::acceptor(Application::io_ctx, { tcp::v4(), 8080 }));
			});
		ENG_LOG_DEBUG("Server listening on port 8080..." << std::endl);

		renderer.registerCommandRecorder([&renderer, &sceneState](VkCommandBuffer commandBuffer) {
			recordCommandsForSceneGraph(renderer, commandBuffer, sceneState);
			});
		renderer.registerUniformBufferProducer([&sceneState]() -> UniformBufferObject {
			return createUniformBufferObject(sceneState);
			});
		renderer.registerModelMatrixBufferUpdateFunction([&sceneState]() -> std::vector<glm::mat4>&{
			updateModelMatrices(sceneState);
			return sceneState.modelMatrices;
			});


		app.mainThreadFunction = [&renderer, &gui]() {gameLoop(renderer, gui); };

		app.start();

		//// Graceful shutdown sequence
		app.shutdown();

	}
	catch (const std::exception& e) {
		ENG_LOG_ERROR("Exception caught by main(): " << e.what() << std::endl);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
