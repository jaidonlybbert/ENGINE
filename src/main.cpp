#include <stdio.h>
#include <renderer/Renderer.hpp>

// Necessary definition for PMP header compilation
#ifndef M_PI
#define M_PI 3.1415926
#endif

#include "logger/Logging.hpp"
#include "sockets/SocketSessionServer.h"
#include "scenes/SceneWorld.hpp"

#include "asio/post.hpp"
#include "asio/io_context.hpp"
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"

#include <thread>
#include <functional>

void stop(asio::io_context& io_context) {
	if (io_context.stopped()) {
		return;
	}

	io_context.stop();
}

void initForVulkan()
{
}

void getDrawData()
{
}

void recordCommandsForSceneGraph(VulkanTemplateApp& app, VkCommandBuffer& commandBuffer)
{
	for (const auto& node : app.sceneState.graph.nodes)
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
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.pipelineFactory->getVkPipeline(shaderId));

		// During scene switching/loading, nodes will be in a partially loaded state not ready to be rendered
		if (app.currentFrame > node.descriptorSetIds.size()) {
			ENG_LOG_DEBUG("Skipping draw for " << node.name << " which has no descriptor sets" << std::endl);
			continue;
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app.pipelineFactory->getVkPipelineLayout(shaderId),
			  0, 1, &app.descriptorSets.get(node.descriptorSetIds.at(app.currentFrame)), 0, nullptr);
		vkCmdPushConstants(commandBuffer, app.pipelineFactory->getVkPipelineLayout(shaderId), VK_SHADER_STAGE_VERTEX_BIT, 0,
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

#include "hid/Input.hpp"

void initWindow(VulkanTemplateApp& app) {
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


int main() {
	
	try {
		printf("Starting app\n");
		// ENG_LOG_TRACE("Application path: " << install_dir.native().c_str() << std::endl);

		VulkanTemplateApp app;

		app.registerInitializationFunction([&app]() {initWindow(app);});
		app.registerInitializationFunction([&app]() {app.initVulkan();});
		app.registerInitializationFunction([&app]() {app.initGui();});
		app.registerInitializationFunction([]() {initLua();});

		app.initialize();

		ENG_LOG_INFO(app);

		asio::io_context io_context;

		asio::post([&app] () {
			app.initializeScene(initializeWorldScene);
		});


		co_spawn(io_context,
			listener(tcp::acceptor(io_context, { tcp::v4(), 8080 })),
			detached);

		std::cout << "Server listening on port 8080..." << std::endl;

		asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&](auto, auto){ stop(io_context); });

		// Run in background thread
		std::thread io_thread([&io_context]() {
			io_context.run();
		});

		app.registerCommandRecorder([&app](VkCommandBuffer commandBuffer) {
			recordCommandsForSceneGraph(app, commandBuffer);
		});
		app.run(); 

		// Graceful shutdown sequence
		stop(io_context);

		if (io_thread.joinable()) {
			io_thread.join();    // Wait for io thread to finish
		}


	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	    	return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
