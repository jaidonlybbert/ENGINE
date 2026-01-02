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

#include "renderer/vk/Renderer.hpp"
#include "logger/Logging.hpp"
#include "sockets/SocketSessionServer.h"
#include "scenes/SceneWorld.hpp"
#include "hid/Input.hpp"
#include "application/Application.hpp"
#include "guis/SceneGui.hpp"
#include "gui/Gui.hpp"
#include "scene/DFT.hpp"


void stop(asio::io_context& io_context) {
	if (io_context.stopped()) {
		return;
	}

	io_context.stop();
}


void recordCommandsForSceneGraph(VkRenderer& renderer, VkCommandBuffer& commandBuffer)
{
	for (const auto& node : renderer.sceneState.graph.nodes)
	{
		if (!node.shaderId.has_value())
		{
			ENG_LOG_TRACE("Skipping draw for " << node.name << " due to no shaderId" << std::endl);
			continue;
		}
		const auto& shaderId = node.shaderId.value();

		if (node.mesh == nullptr)
		{	
			ENG_LOG_TRACE("Skipping draw for " << node.name << " due to no mesh" << std::endl);
			continue;
		}

		if (!node.visible)
		{
			ENG_LOG_TRACE("Skipping draw for " << node.name << "due to visibility set to false" << std::endl);
			continue;
		}
		ENG_LOG_TRACE("Drawing " << node.name << std::endl);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipelineFactory->getVkPipeline(shaderId));

		// During scene switching/loading, nodes will be in a partially loaded state not ready to be rendered
		if (renderer.currentFrame > node.descriptorSetIds.size()) {
			ENG_LOG_DEBUG("Skipping draw for " << node.name << " which has no descriptor sets" << std::endl);
			continue;
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipelineFactory->getVkPipelineLayout(shaderId),
			  0, 1, &renderer.descriptorSets.get(node.descriptorSetIds.at(renderer.currentFrame)), 0, nullptr);
		vkCmdPushConstants(commandBuffer, renderer.pipelineFactory->getVkPipelineLayout(shaderId), VK_SHADER_STAGE_VERTEX_BIT, 0,
			sizeof(uint32_t), &node.nodeId);


		auto* meshPtr = node.mesh;
		if (dynamic_cast<ENG::Mesh<ENG::VertexPosColTex>*>(meshPtr))
		{
			auto* castPtr = dynamic_cast<ENG::Mesh<ENG::VertexPosColTex>*>(meshPtr);
			VkBuffer vertexBuffers[] = {castPtr->vertexBuffer->buffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, castPtr->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(castPtr->indices.size()), 1, 0, 0, 0);
		}
		else if (dynamic_cast<ENG::Mesh<ENG::VertexPosNorTex>*>(meshPtr))
		{	
			ENG_LOG_TRACE("Cast for " << node.name << " success" << std::endl);
			auto* castPtr = dynamic_cast<ENG::Mesh<ENG::VertexPosNorTex>*>(meshPtr);
			VkBuffer vertexBuffers[] = {castPtr->vertexBuffer->buffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, castPtr->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(castPtr->indices.size()), 1, 0, 0, 0);
		}
		else if (dynamic_cast<ENG::Mesh<ENG::VertexPos>*>(meshPtr))
		{
			ENG_LOG_TRACE("Cast for " << node.name << " success" << std::endl);
			auto* castPtr = dynamic_cast<ENG::Mesh<ENG::VertexPos>*>(meshPtr);
			assert(castPtr != nullptr);
			assert(castPtr->vertexBuffer != nullptr);
			assert(castPtr->vertexBuffer->buffer != nullptr);
			assert(castPtr->indexBuffer != nullptr);
			assert(castPtr->indexBuffer->buffer != nullptr);
			VkBuffer vertexBuffers[] = {castPtr->vertexBuffer->buffer};

			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, castPtr->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(castPtr->indices.size()), 1, 0, 0, 0);
		}
		else if (dynamic_cast<ENG::Mesh<ENG::VertexPosNorCol>*>(meshPtr))
		{
			ENG_LOG_TRACE("Cast for " << node.name << " success" << std::endl);
			auto* castPtr = dynamic_cast<ENG::Mesh<ENG::VertexPosNorCol>*>(meshPtr);
			assert(castPtr != nullptr);
			assert(castPtr->vertexBuffer != nullptr);
			assert(castPtr->vertexBuffer->buffer != nullptr);
			assert(castPtr->indexBuffer != nullptr);
			assert(castPtr->indexBuffer->buffer != nullptr);
			VkBuffer vertexBuffers[] = {castPtr->vertexBuffer->buffer};

			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdDraw(commandBuffer, static_cast<uint32_t>(castPtr->vertices.size()), 1, 0, 0);
		}
	}
}


#include "lua.hpp"

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

#include "gui/Gui.hpp"

void initWindow(VkRenderer& app) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	app.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(app.window, &app.sceneState);
	glfwSetFramebufferSizeCallback(app.window, app.framebufferResizeCallback);
	glfwGetCursorPos(app.window, &app.sceneState.cursor_x, &app.sceneState.cursor_y);
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
		Gui gui;
		SceneGui sceneGui;

		VkRenderer renderer{
			{
				[&renderer]() {initWindow(renderer);},
				[&renderer]() {renderer.initVulkan();},
				[&renderer]() {renderer.initGui();},
				[]() {initLua();}
			}
		};

		gui.registerDrawCall([&renderer, &sceneGui]() {sceneGui.drawGui(renderer.sceneState);});

		ENG_LOG_DEBUG(renderer);

		app.registerInitFunction("renderer.initializeScene()", [&renderer]() { renderer.initializeScene(initializeWorldScene); });
		app.registerCoroutineFunction("listener(tcp::acceptor)", []() {
			return listener(tcp::acceptor(Application::io_ctx, { tcp::v4(), 8080 }));
			});
		ENG_LOG_DEBUG("Server listening on port 8080..." << std::endl);

		renderer.registerCommandRecorder([&renderer](VkCommandBuffer commandBuffer) {
			recordCommandsForSceneGraph(renderer, commandBuffer);
			});

		renderer.registerUniformBufferProducer([&renderer]() -> UniformBufferObject {
			return createUniformBufferObject(renderer.sceneState);
			});

		renderer.registerModelMatrixBufferUpdateFunction([&renderer]() -> std::vector<glm::mat4>&{
			updateModelMatrices(renderer.sceneState);
			return renderer.sceneState.modelMatrices;
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
