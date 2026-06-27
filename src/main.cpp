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
#include "events/Event.hpp"

#include "lua.hpp"

#include "physics/physics.hpp"

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

void initWindow(VkRenderer& app, WindowUserData& windowUserData) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	app.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(app.window, &windowUserData);
	glfwSetFramebufferSizeCallback(app.window, InputController::framebufferResizeCallback);
	glfwGetCursorPos(app.window, &windowUserData.cursorXScreenCoords, &windowUserData.cursorYScreenCoords);
	glfwGetWindowSize(app.window, &windowUserData.windowWidthScreenCoords, &windowUserData.windowHeightScreenCoords);
	glfwSetScrollCallback(app.window, InputController::mouse_scroll_callback);
	glfwSetKeyCallback(app.window, InputController::key_callback);
	glfwSetMouseButtonCallback(app.window, InputController::mouse_button_callback);
	glfwSetCursorPosCallback(app.window, InputController::mouse_movement_callback);
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


AABB findExtremaVertices(std::vector<VertexPosNorCol>& verts) {
	const auto& minXIt = std::min_element(verts.begin(), verts.end(), [](const VertexPosNorCol& rhs, const VertexPosNorCol& lhs) {
		return (rhs.pos.x < lhs.pos.x);
	});
	const auto& minYIt = std::min_element(verts.begin(), verts.end(), [](const VertexPosNorCol& rhs, const VertexPosNorCol& lhs) {
		return (rhs.pos.y < lhs.pos.y);
	});
	const auto& minZIt = std::min_element(verts.begin(), verts.end(), [](const VertexPosNorCol& rhs, const VertexPosNorCol& lhs) {
		return (rhs.pos.z < lhs.pos.z);
	});
	const auto& maxXIt = std::max_element(verts.begin(), verts.end(), [](const VertexPosNorCol& rhs, const VertexPosNorCol& lhs) {
		return (rhs.pos.x < lhs.pos.x);
	});
	const auto& maxYIt = std::max_element(verts.begin(), verts.end(), [](const VertexPosNorCol& rhs, const VertexPosNorCol& lhs) {
		return (rhs.pos.y < lhs.pos.y);
	});
	const auto& maxZIt = std::max_element(verts.begin(), verts.end(), [](const VertexPosNorCol& rhs, const VertexPosNorCol& lhs) {
		return (rhs.pos.z < lhs.pos.z);
	});

	const auto& maxX = maxXIt->pos.x;
	const auto& maxY = maxYIt->pos.y;
	const auto& maxZ = maxZIt->pos.z;
	const auto& minX = minXIt->pos.x;
	const auto& minY = minYIt->pos.y;
	const auto& minZ = minZIt->pos.z;

	const AABB& aabb{
		{minX, minY, minZ, 1},
		{maxX, maxY, maxZ, 1}
	};

	return aabb;
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

void handleNodeRotationPreserveYAsUpAction(const ClientHidEvent& hidEvent, SceneState& sceneState)
{
	if (sceneState.activeNodeIdx >= sceneState.graph.nodes.size())
	{
		ENG_LOG_ERROR("Active node idx is invalid!" << std::endl);
		return;
	}
	auto& activeNode = sceneState.graph.nodes.at(sceneState.activeNodeIdx);

	node_rotation_follows_input_preserve_y_as_up(activeNode, hidEvent.look_dx, hidEvent.look_dy);
}

void handleHidEvent(const ClientHidEvent& hidEvent, SceneState& sceneState)
{
	for (const auto& action : hidEvent.actions)
	{
		if (action == Action::NODE_ROTATION_PRESERVE_Y_AS_UP)
		{
			handleNodeRotationPreserveYAsUpAction(hidEvent, sceneState);
		} 
	}
}

void castRayForMouseHoverOnNode(const ENG::Node& node, const SceneState& sceneState, const glm::vec3& rayOrigin, const glm::vec3& rayDir)
{
	static std::vector<bool> nodeHoverMap(sceneState.graph.nodes.size(), false);
	if (node.nodeId >= nodeHoverMap.size())
	{
		ENG_LOG_DEBUG("nodeHoverMap too small" << std::endl);
		return;
	}

	const auto& boundingSphereCenter = glm::vec3(sceneState.modelMatrices.at(node.nodeId)[3]); // location
	const auto& boundingSphereRadius = 1.f;  // from generation algorithm

	const auto& rayOriginToSphereCenter = boundingSphereCenter - rayOrigin;
	const auto& dotProduct = glm::dot(rayOriginToSphereCenter, rayDir);

	if (dotProduct < 0)
	{
		ENG_LOG_DEBUG(node.name << " is behind the camera!" << std::endl);
		return;
	}

	// standard distance formula for shortest distance from point to line
	const auto& crossProduct = glm::cross(rayOriginToSphereCenter, rayDir);
	const auto& distance = glm::length(crossProduct) / glm::length(rayDir);

	if (distance < 1.f && !nodeHoverMap.at(node.nodeId))
	{
		nodeHoverMap.at(node.nodeId) = true;
		ENG_LOG_DEBUG("Cursor is on " << node.name << std::endl);
	}
	else if (distance > 1.f && nodeHoverMap.at(node.nodeId))
	{
		nodeHoverMap.at(node.nodeId) = false;
		ENG_LOG_DEBUG("Cursor is not on " << node.name << std::endl);
	}
}

void castRayForMouseHover(const WindowUserData& windowUserData, const SceneState& sceneState, const UniformBufferObject& ubo)
{
	static bool cursorIsHoveringOnSphere = false;
	const auto& m = ubo.model; // 4x4 matrix representing rotation, translation, scale
	const auto& v = ubo.view;  // 4x4 matrix representing camera rotation, translation
	const auto& p = ubo.proj;  // 4x4 matrix representing the camera lens projection

	// "normalized device coordinates (NDC)" the coordinates centered around 0,0 on the screen ranging from -1,1 to 1,-1 top-left to bottom-right
	const auto& mouseXNDC = (2.f * windowUserData.cursorXScreenCoords) / windowUserData.windowWidthScreenCoords - 1.0f;
	const auto& mouseYNDC = 1.f - (2.f * windowUserData.cursorYScreenCoords) / (windowUserData.windowHeightScreenCoords);

	// a vector of 4x4 model matrices representing the rotation, translation, and scale of each model in the scene, indexed by nodeId
	const auto& modelMatrices = sceneState.modelMatrices;

	// calculate ray origin and direction for ray function r(t) = origin + t * direction
	glm::mat4 invVP = glm::inverse(p * v);
	glm::vec4 screenPos = glm::vec4(mouseXNDC, mouseYNDC, -1.0f, 1.0f);  // near plane
	glm::vec4 worldPos = invVP * screenPos;  // "un-project" mouse position
	worldPos /= worldPos.w;

	glm::vec3 rayOrigin = glm::vec3(glm::inverse(v)[3]); // Camera position
	glm::vec3 rayDir = glm::normalize(glm::vec3(worldPos) - rayOrigin);

	// iterate over visible nodes and check for hover
	for (const auto& node : sceneState.graph.nodes)
	{
		if (node.selectable)
		{
			castRayForMouseHoverOnNode(node, sceneState, rayOrigin, rayDir);
		}
	}
}

void handleHIDEvents(std::deque<ClientHidEvent>& eventQueue, SceneState& sceneState) {
	while (!eventQueue.empty())
	{
		const auto& clientEvent = eventQueue.front();
		handleHidEvent(clientEvent, sceneState);
		eventQueue.pop_front();
	}
}

void mesh_bind_event_handler(VkRenderer& renderer, SceneState& sceneState, VkAdapter& adapter, BindHostMeshDataEvent&& bindEvent) {
	auto& hostMesh = bindEvent.meshData;
	auto& node = get_node_by_id(sceneState.graph, bindEvent.nodeId);

	if (hostMesh.texturePath.has_value())
	{
		if (!renderer.textureImages.contains(hostMesh.texturePath.value()))
		{
			renderer.createTexture(hostMesh.texturePath.value());
		}
	}

	const auto drawIdx = adapter.emplaceDrawData(
		{
			DrawDataProperties::CLEAR,
			{bindEvent.nodeId},
			hostMesh.texturePath,
			{std::nullopt},  // no descriptor sets
			adapter.create_draw_data(
				std::move(hostMesh.vertexBuffer),
				std::move(hostMesh.indexBuffer)
			),
		}
	);

	//initializeBoundingBox(sceneState, node);

	node.shaderId = bindEvent.meshData.shaderId;
	node.draw_data_idx = drawIdx;
	ENG_LOG_TRACE("Node: " << node.name << " DrawDataIndex: " << drawIdx << std::endl);

	adapter.graphicsEventQueue.push(
		CommandCompletionEvent {
			[&adapter, &node, drawIdx] {
				adapter.set_property(drawIdx, DrawDataProperties::INDEX_BUFFERS_INITIALIZED);
				adapter.set_property(drawIdx, DrawDataProperties::VERTEX_BUFFERS_INITIALIZED);
				adapter.createDescriptorSets(drawIdx, node);
				adapter.set_property(drawIdx, DrawDataProperties::DESCRIPTOR_SETS_INITIALIZED);
				ENG_LOG_TRACE("Created draw data for " << node.name << std::endl);
			}
		}
	);
}

void handleGraphicsEvents(VkRenderer& renderer, VkAdapter& adapter, SceneState& sceneState)
{
	while (!adapter.graphicsEventQueue.empty()) {
		GraphicsEvent graphicsEvent{ adapter.graphicsEventQueue.pop() };
		
		if (std::holds_alternative<BindHostMeshDataEvent>(graphicsEvent))
		{
			mesh_bind_event_handler(renderer, sceneState, adapter, std::move(std::get<BindHostMeshDataEvent>(graphicsEvent)));
		}
		else if (std::holds_alternative<CommandRecorderEvent>(graphicsEvent))
		{
			adapter.command_recorder_event_handler(std::move(std::get<CommandRecorderEvent>(graphicsEvent)));
		}
		else if (std::holds_alternative<CommandCompletionEvent>(graphicsEvent))
		{
			std::get<CommandCompletionEvent>(graphicsEvent).commandCompletionHandler();
		}
	}

}

void gameLoop(VkAdapter& adapter, VkRenderer& renderer, Gui& gui, WindowUserData& windowUserData, SceneState& sceneState) {
	while(!glfwWindowShouldClose(renderer.window)) {
		profilerMarkStart("run_frame");

		glfwPollEvents();
		handleHIDEvents(windowUserData.eventQueue, sceneState);

		handleGraphicsEvents(renderer, adapter, sceneState);

		gui.drawGui();
		renderer.drawFrame();

		profilerMarkStop("run_frame");
	}

	vkDeviceWaitIdle(renderer.device);
}

int main() {
	
	try {
		ENG_LOG_TRACE("Starting app" << std::endl);

		WindowUserData windowUserData;

		VkRenderer renderer{
			windowUserData.windowResized,
			{
				[&renderer, &windowUserData]() {initWindow(renderer, windowUserData);},
				[&renderer]() {renderer.initVulkan();},
				[&renderer]() {renderer.initGui();},
				[]() {initLua();}
			},
			{
				[]() {lua_close(luaState); },
				[&renderer]() { renderer.cleanupGui(); },
				[&renderer]() { renderer.cleanupVulkan(); },
				[&renderer]() { renderer.cleanupWindow(); }
			}
		};

		Gui gui;
		SceneState sceneState;
		SceneGui sceneGui;

		gui.registerDrawCall([&sceneGui, &sceneState]() {sceneGui.drawGui(sceneState);});

		ENG_LOG_DEBUG(renderer);

		VkAdapter renderAdapter{ renderer };

		Application app;
		app.registerInitFunction("renderer.initializeScene()", [&renderer, &sceneState, &renderAdapter]() { 
			renderer.sceneReadyToRender = false;
			initializeWorldScene(renderer, renderAdapter, sceneState);
			renderer.sceneReadyToRender = true;
			sceneState.initialized = true;
			});

		app.registerCoroutine("listener(tcp::acceptor)", []() {
			return listener(tcp::acceptor(Application::io_ctx, { tcp::v4(), 8080 }));
			});

		app.registerDedicatedThread("physics.run_physics()", [&sceneState]() {
			using namespace std::chrono_literals;
			while (!sceneState.initialized) { std::this_thread::sleep_for(500ms); }
			run_physics();
			});
		ENG_LOG_DEBUG("Server listening on port 8080..." << std::endl);


		renderer.registerCommandRecorder([&renderAdapter, &renderer, &sceneState](VkCommandBuffer commandBuffer) {
			renderAdapter.recordCommandsForSceneGraph2(renderer, commandBuffer, sceneState);
			});
		renderer.registerUniformBufferProducer([&sceneState]() -> UniformBufferObject {
			return createUniformBufferObject(sceneState);
			});
		renderer.registerModelMatrixBufferUpdateFunction([&sceneState]() -> std::vector<glm::mat4>&{
			updateModelMatrices(sceneState);
			return sceneState.modelMatrices;
			});
		/*
		renderer.registerUniformBufferConsumer([&sceneState, &windowUserData](const UniformBufferObject& ubo) {
			castRayForMouseHover(windowUserData, sceneState, ubo);
			});
		*/


		app.mainThreadFunction = [&renderAdapter, &renderer, &gui, &windowUserData, &sceneState]() {
			gameLoop(renderAdapter, renderer, gui, windowUserData, sceneState); 
		};

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
