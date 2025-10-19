#include<iostream>
#include<iterator>
#include<stack>
#include<locale>
#include<vector>
#include<queue>
#include<stdexcept>
#include<cstdlib>
#include<cstring>
#include<string>
#include<optional>
#include<set>
#include<algorithm>
#include<limits>
#include<cstdint>
#include<fstream>
#include<filesystem>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <chrono>
#include "vulkan/vulkan_core.h"
#include "GLFW/glfw3.h"

#include<tiny_gltf.h>
#include<tiny_obj_loader.h>
#include<stb_image.h>

#ifdef _WIN32
#include "tracy/Tracy.hpp"
#endif

// Necessary definition for PMP header compilation
#define M_PI 3.1415926
#include "pmp/surface_mesh.h"
#include "pmp/algorithms/triangulation.h"
#include "pmp/algorithms/shapes.h"
#include "pmp/algorithms/utilities.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "EngineConfig.hpp"
#include "Utils.hpp"
#include "primitives/Mesh.hpp"
#include "pipelines/ShaderFactory.hpp"
#include "pipelines/PipelineFactory.hpp"
#include "interfaces/FilesystemInterface.hpp"
#include "interfaces/Command.hpp"
#include "interfaces/Swapchain.hpp"
#include "interfaces/Device.hpp"
#include "interfaces/PhysicalDevice.hpp"
#include "interfaces/Image.hpp"
#include "interfaces/Obj.hpp"
#include "interfaces/Scene.hpp"
#include "interfaces/Gltf.hpp"
#include "interfaces/Instance.hpp"
#include "interfaces/Buffer.hpp"
#include "interfaces/Logging.hpp"
#include "interfaces/Gui.hpp"


namespace ENG
{

template <typename T>
class Pool {
public:
	Pool(const size_t capacity)
	{
		data.reserve(capacity);
	}

	std::pair<size_t, T&> emplace_back()
	{
		if (handle.capacity() < id_counter)
		{
			throw std::runtime_error("Pool capacity exceeded!");
		}
		T& ref = handle.emplace_back();
		auto id = id_counter;
		id_counter++;
		return { id, ref };
	}

	T& get(const size_t id)
	{
		assert(id < handle.size());
		return handle.at(id);
	}

private:
	std::vector<T> data;
	std::vector<T>& handle = data;
	inline static unsigned long long id_counter = 0;
	Pool() = delete;
	Pool(const Pool& other) = delete;
	Pool& operator=(const Pool& other) = delete;
};
	
template class Pool<VkDescriptorSet>;
}

using namespace ENG;


struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

class VulkanTemplateApp {
public:
	VulkanTemplateApp() {
		initWindow();
		initVulkan();
		initGui();
	}

	void run() {
		while(!glfwWindowShouldClose(window)) {
#ifdef _WIN32
			FrameMarkStart("run_frame");
#endif
			//glfwGetCursorPos(window, &sceneState.cursor_x, &sceneState.cursor_y);
			ENG_LOG_TRACE("glfwPollEvents" << std::endl);
			glfwPollEvents();
			ENG_LOG_TRACE("drawGUI" << std::endl);
			drawGUI();
			ENG_LOG_TRACE("drawFrame" << std::endl);
			drawFrame();
#ifdef _WIN32
			FrameMarkEnd("run_frame");
#endif

		}

		vkDeviceWaitIdle(device);
	}

	~VulkanTemplateApp() {
		faceColorBuffers.clear();
		faceIdMapBuffers.clear();
		sceneState.posColTexMeshes.clear();
		sceneState.posNorTexMeshes.clear();
		sceneState.posMeshes.clear();
		sceneState.posNorColMeshes.clear();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		cleanupGui();

		swapchain->cleanupSwapChain(device);

		vkDestroySampler(device, textureSampler, nullptr);
		vkDestroyImageView(device, textureImageView, nullptr);
		vkDestroyImage(device, textureImage, nullptr);
		vkFreeMemory(device, textureImageMemory, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		uniformBuffers.clear();
		modelMatrixBuffers.clear();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		commands.reset();
		pipelineFactory.reset();

		if (enableValidationLayers) {
			ENG::InstanceFactory::DestroyDebugUtilsMessengerEXT(instanceFactory->instance, instanceFactory->debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instanceFactory->instance, surface, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroyInstance(instanceFactory->instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}


	void cleanupGui() {
		vkDestroyDescriptorPool(device, imguiPool, nullptr);
	}

	friend std::ostream& operator<<(std::ostream& os, VulkanTemplateApp& app) {
		// Print application name and version
		ENG_LOG_INFO(PROJECT_NAME_AND_VERSION << std::endl);
		// Print physical device properties
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(app.physicalDevice, &deviceProperties);
		ENG_LOG_INFO(std::endl << "Physical Device Properties: " << std::endl);
		ENG_LOG_INFO("Name:\t" << deviceProperties.deviceName << std::endl);
		ENG_LOG_INFO("API Version:\t" << deviceProperties.apiVersion << std::endl);
		ENG_LOG_INFO("Driver Version:\t" << deviceProperties.driverVersion << std::endl);
		ENG_LOG_INFO(std::endl);
		
		// Print available extensions
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
		ENG_LOG_INFO("Available Vulkan Extensions:" << std::endl);
		for (const auto& extension: availableExtensions) {
			ENG_LOG_INFO('\t' << extension.extensionName << std::endl);
		}

		// Print used extensions
		auto enabledExtensions = app.instanceFactory->getRequiredExtensions();

		ENG_LOG_INFO("Enabled Vulkan Extensions:" << std::endl);
		for (auto extension : enabledExtensions) {
			ENG_LOG_INFO('\t' << extension << std::endl);
		}
		return os;
	}


	SceneState sceneState;
	GLFWwindow* window;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;
	VkRenderPass renderPass;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	bool framebufferResized = false;
	uint32_t currentFrame = 0;
	std::vector<ENG::Buffer> uniformBuffers;
	std::vector<void*> uniformBuffersMapped;
	std::vector<ENG::Buffer> modelMatrixBuffers;
	std::vector<void*> modelMatrixBuffersMapped;
	std::vector<ENG::Buffer> faceColorBuffers;
	std::vector<ENG::Buffer> faceIdMapBuffers;
	std::vector<void*> faceColorBuffersMapped;
	std::vector<void*> faceIdMapBuffersMapped;
	VkDescriptorPool descriptorPool;
	VkDescriptorPool imguiPool;
	Pool<VkDescriptorSet> descriptorSets{ 10 };
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	std::unique_ptr<ENG::InstanceFactory> instanceFactory;
	std::unique_ptr<ENG::PipelineFactory> pipelineFactory;
	std::unique_ptr<ENG::Command> commands;
	std::unique_ptr<ENG::Swapchain> swapchain;

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_T && action == GLFW_PRESS)
		{
			ENG_LOG_INFO("Toggle settings window visibility" << std::endl);
			auto* sceneState = static_cast<SceneState*>(glfwGetWindowUserPointer(window));
			sceneState->settings.showSettings = !sceneState->settings.showSettings;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS)
		{
			ENG_LOG_INFO("E key down" << std::endl);
			auto* sceneState = static_cast<SceneState*>(glfwGetWindowUserPointer(window));
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
			ENG_LOG_INFO("R key down" << std::endl);
			auto* sceneState = static_cast<SceneState*>(glfwGetWindowUserPointer(window));
			// auto& test_rot = sceneState->test_model;
			// rotate 3 degrees around y-axis when E is pressed
			// auto dx = glm::angleAxis(glm::radians(3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			// test_rot = glm::mat4_cast(dx) * test_rot;
		}
	}

	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		auto* sceneState = static_cast<SceneState*>(glfwGetWindowUserPointer(window));

		if (button == GLFW_MOUSE_BUTTON_MIDDLE)
		{
			if (action == GLFW_PRESS) // && initial_press)
			{
				glfwGetCursorPos(window, &sceneState->cursor_x, &sceneState->cursor_y);
				ENG_LOG_INFO("Middle mouse initial press" << std::endl);
			}
			else // action is GLFW_RELEASE
			{
				ENG_LOG_INFO("Middle mouse released" << std::endl);
			}
		}
	}

	static void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{
		static double dx, dy = 0.f;
		auto* sceneState = static_cast<SceneState*>(glfwGetWindowUserPointer(window));
		// Print the scroll offsets
		printf("Scroll Offset - X: %.2f, Y: %.2f\n", xoffset, yoffset);

		const auto& shift_state = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
		if (shift_state == GLFW_PRESS)
		{
			ENG_LOG_INFO("Shift down" << std::endl);
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


	static void mouse_movement_callback(GLFWwindow* window, double xpos, double ypos)
	{
		static double dx, dy = 0.f;
		dx = 0.f;
		dy = 0.f;

		auto* sceneState = static_cast<SceneState*>(glfwGetWindowUserPointer(window));
		const auto& middle_mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);
		if (middle_mouse_state == GLFW_PRESS)
		{
			ENG_LOG_INFO("Middle mouse down" << std::endl);
			dx = xpos - sceneState->cursor_x;
			dy = ypos - sceneState->cursor_y;
			sceneState->cursor_x = xpos;
			sceneState->cursor_y = ypos;

			ENG_LOG_INFO("dx: " << dx << " dy: " << dx << std::endl);
			constexpr float sensitivity = 0.1f;

			if (sceneState->activeNodeIdx >= sceneState->graph.nodes.size())
			{
				ENG_LOG_ERROR("Active node idx is invalid!" << std::endl);
				return;
			}
			auto& activeNode = sceneState->graph.nodes.at(sceneState->activeNodeIdx);

			auto dx_radians = glm::angleAxis(glm::radians(static_cast<float>(dx) * sensitivity), glm::vec3(0.0f, 0.0f, 1.0f));
			activeNode.rotation	= dx_radians * activeNode.rotation;
			const auto& local_x_axis = glm::normalize(activeNode.rotation * glm::vec3(1.0f, 0.0f, 0.0f));
			auto dy_radians = glm::angleAxis(glm::radians(static_cast<float>(dy) * sensitivity), glm::vec3(local_x_axis.x, local_x_axis.y, local_x_axis.z));
			activeNode.rotation = dy_radians * activeNode.rotation;

			// auto& test_rot = sceneState->test_model;
			// rotate 3 degrees around y-axis when E is pressed
			//auto dx = glm::angleAxis(glm::radians(3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			// test_rot = glm::mat4_cast(dy_radians) * glm::mat4_cast(dx_radians) * test_rot;
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

	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, &sceneState);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		glfwGetCursorPos(window, &sceneState.cursor_x, &sceneState.cursor_y);
		glfwSetScrollCallback(window, mouse_scroll_callback);
		glfwSetKeyCallback(window, key_callback);
		glfwSetMouseButtonCallback(window, mouse_button_callback);
		glfwSetCursorPosCallback(window, mouse_movement_callback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<VulkanTemplateApp*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	void initVulkan() {
		instanceFactory = std::make_unique<ENG::InstanceFactory>();
		instanceFactory->createInstance();
		instanceFactory->setupDebugMessenger();
		createSurface();
		ENG::PhysicalDevice::pickPhysicalDevice(instanceFactory->instance, physicalDevice, surface);
		ENG::Device::createLogicalDevice(surface, physicalDevice, validationLayers, graphicsQueue, presentQueue, device);
		swapchain = std::make_unique<Swapchain>(physicalDevice, surface, device, *window);
		pipelineFactory = std::make_unique<ENG::PipelineFactory>(device, swapchain->swapChainImageFormat, findDepthFormat(physicalDevice));
		renderPass = pipelineFactory->getRenderPass();
		commands = std::make_unique<Command>(physicalDevice, device, surface); // creates command pool
		createDepthResources(device, physicalDevice, swapchain->swapChainExtent, swapchain->depthImage, swapchain->depthImageMemory, swapchain->depthImageView);
		swapchain->createFramebuffers(renderPass, device);
		createTextureImage();
		createTextureImageView();
		createTextureSampler();
		createUniformBuffers();
		createDescriptorPool();
		commands->createCommandBuffers(device);
		createSyncObjects();
	}

	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
				VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}


	void createSurface() {
		if (glfwCreateWindowSurface(instanceFactory->instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}


	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		    throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapchain->swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapchain->swapChainExtent;
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchain->swapChainExtent.width);
		viewport.height = static_cast<float>(swapchain->swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapchain->swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		for (const auto& node : sceneState.graph.nodes)
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
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineFactory->getVkPipeline(shaderId));

			assert(currentFrame < node.descriptorSetIds.size());
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineFactory->getVkPipelineLayout(shaderId),
				  0, 1, &descriptorSets.get(node.descriptorSetIds.at(currentFrame)), 0, nullptr);
			vkCmdPushConstants(commandBuffer, pipelineFactory->getVkPipelineLayout(shaderId), VK_SHADER_STAGE_VERTEX_BIT, 0,
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

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		vkCmdEndRenderPass(commandBuffer);
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		    throw std::runtime_error("failed to record command buffer!");
		}
	}

	void drawFrame() {
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		uint32_t imageIndex;

		VkResult result = vkAcquireNextImageKHR(device, swapchain->swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			swapchain->recreateSwapChain(physicalDevice, device, surface, window, renderPass);
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		vkResetCommandBuffer(commands->commandBuffers[currentFrame], 0);
		recordCommandBuffer(commands->commandBuffers[currentFrame], imageIndex);
		updateUniformBuffer(currentFrame);
		updateModelMatrixBuffer();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &(commands->commandBuffers[currentFrame]);

		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		    throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = {swapchain->swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			swapchain->recreateSwapChain(physicalDevice, device, surface, window, renderPass);
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create semaphores!");
			}
		}
	}

	void createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffers.reserve(2);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			uniformBuffers.emplace_back(device, physicalDevice, bufferSize, bufferSize,
			       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			vkMapMemory(device, uniformBuffers[i].bufferMemory, 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	/// <summary>
	/// Must be called after all nodes are loaded
	/// </summary>
	void createModelMatrices()
	{
		VkDeviceSize bufferSize = sizeof(glm::mat4) * sceneState.graph.nodes.size();
		modelMatrixBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
		modelMatrixBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			ENG_LOG_DEBUG("Creating " << i << " model buffer of size " << bufferSize << std::endl);
			modelMatrixBuffers.emplace_back(device, physicalDevice, sizeof(glm::mat4), bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			vkMapMemory(device, modelMatrixBuffers[i].bufferMemory, 0, bufferSize, 0, &modelMatrixBuffersMapped[i]);
		}
	}

	void createFaceIdBuffers(const uint32_t number_of_faces)
	{
		VkDeviceSize bufferSize = sizeof(uint32_t) * number_of_faces;
		faceIdMapBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
		faceIdMapBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			ENG_LOG_DEBUG("Creating FaceID buffer " << i << " of size " << bufferSize << std::endl);
			faceIdMapBuffers.emplace_back(device, physicalDevice, sizeof(uint32_t), bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			vkMapMemory(device, faceIdMapBuffers[i].bufferMemory, 0, bufferSize, 0, &faceIdMapBuffersMapped[i]);
		}
	}

	void createFaceColorBuffers(const uint32_t number_of_faces)
	{
		VkDeviceSize bufferSize = sizeof(glm::vec4) * number_of_faces;
		faceColorBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
		faceColorBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			ENG_LOG_DEBUG("Creating FaceColor buffer " << i << " of size " << bufferSize << std::endl);
			faceColorBuffers.emplace_back(device, physicalDevice, sizeof(glm::vec4), bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			vkMapMemory(device, faceColorBuffers[i].bufferMemory, 0, bufferSize, 0, &faceColorBuffersMapped[i]);
		}
	}

	void updateModelMatrix(glm::mat4& modelMatrix, const ENG::Node& node)
	{
		// Compute local transform from TRS data
		modelMatrix = glm::translate(glm::mat4(1.f), node.translation)
			* glm::mat4_cast(node.rotation)
			* glm::scale(glm::mat4(1.f), node.scale);
	}

	class DFIterator {
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = ENG::Node*;
		using difference_type = std::ptrdiff_t;
		using pointer = ENG::Node**;
		using reference = ENG::Node*&;

		DFIterator() = default;
		explicit DFIterator(ENG::Node* root) {
			if (root) stack.emplace(Frame{ root, 0 });
		}

		ENG::Node* operator*() const {
			return stack.empty() ? nullptr : stack.top().node;
		}

		DFIterator& operator++() {
			if (stack.empty()) return *this;

			Frame& top = stack.top();
			ENG::Node* current = top.node;

			if (top.childIndex < current->children.size()) {
				ENG::Node* child = current->children[top.childIndex++];
				stack.emplace(Frame{ child, 0 });
			}
			else {
				stack.pop();
				if (!stack.empty()) ++(*this); // Continue to next sibling or backtrack
			}

			return *this;
		}

		bool operator!=(const DFIterator& other) const {
			return stack != other.stack;
		}

	private:
		struct Frame {
			ENG::Node* node;
			size_t childIndex;
			bool operator==(const Frame& other) const {
				return node == other.node && childIndex == other.childIndex;
			}
		};

		std::stack<Frame> stack;
	};

	class DFTraversal {
	public:
		explicit DFTraversal(ENG::Node* root) : root(root) {}

		DFIterator begin() const { return DFIterator(root); }
		DFIterator end() const { return DFIterator(); }

	private:
		ENG::Node* root;
	};

	void updateModelMatrixBuffer()
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

		const auto& bufferSize = sceneState.modelMatrices.size() * sizeof(glm::mat4);
		memcpy(modelMatrixBuffersMapped[currentFrame], sceneState.modelMatrices.data(), bufferSize);
	}

	void updateUniformBuffer(uint32_t currentImage) {
		UniformBufferObject ubo{};

		auto& cameraNode = sceneState.graph.nodes.at(sceneState.activeCameraNodeIdx);
		auto* cameraPtr = get_active_camera(sceneState);

		// TODO: create buffer for model matrices
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		const glm::vec3& cam_pos = cameraNode.translation;

		ubo.view = glm::lookAt(cam_pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

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

		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}

	void createDescriptorPool() {
		std::array<VkDescriptorPoolSize, 3> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 100;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 100;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 100;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 100;

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	VkWriteDescriptorSet createDescriptorWriteModelMatrix(const ENG::Node& node, const size_t frameIdx, const size_t bindingIdx, const VkDescriptorBufferInfo& modelMatrixBufferInfo) {
		assert(node.descriptorSetIds.size() > frameIdx);
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets.get(node.descriptorSetIds.at(frameIdx));
		descriptorWrite.dstBinding = bindingIdx;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &modelMatrixBufferInfo;

		return descriptorWrite;
	}

	VkWriteDescriptorSet createDescriptorWriteSampler(const ENG::Node& node, const size_t frameIdx, const size_t bindingIdx, const VkDescriptorImageInfo& imageInfo) {
		assert(node.descriptorSetIds.size() > frameIdx);
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets.get(node.descriptorSetIds.at(frameIdx));
		descriptorWrite.dstBinding = bindingIdx;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		return descriptorWrite;
	}

	VkWriteDescriptorSet createDescriptorWriteUbo(const ENG::Node& node, const size_t frameIdx, const size_t bindingIdx, const VkDescriptorBufferInfo& bufferInfo) {
		assert(node.descriptorSetIds.size() > frameIdx);
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets.get(node.descriptorSetIds.at(frameIdx));
		descriptorWrite.dstBinding = bindingIdx;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		return descriptorWrite;
	}

	VkWriteDescriptorSet createDescriptorWriteFaceColorMatrix(const ENG::Node& node, const size_t frameIdx, const size_t bindingIdx, const VkDescriptorBufferInfo& bufferInfo)
	{
		assert(node.descriptorSetIds.size() > frameIdx);
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets.get(node.descriptorSetIds.at(frameIdx));
		descriptorWrite.dstBinding = bindingIdx;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		return descriptorWrite;
	}

	VkWriteDescriptorSet createDescriptorWriteFaceIdMapBuffer(const ENG::Node& node, const size_t frameIdx, const size_t bindingIdx, const VkDescriptorBufferInfo& bufferInfo)
	{
		assert(node.descriptorSetIds.size() > frameIdx);
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets.get(node.descriptorSetIds.at(frameIdx));
		descriptorWrite.dstBinding = bindingIdx;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		return descriptorWrite;
	}

	void writeDescriptorSets(const ENG::Node& node) {
		// NOTE: This seems overly complicated and inefficient?
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i].buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView;
			imageInfo.sampler = textureSampler;

			VkDescriptorBufferInfo modelMatrixBufferInfo{};
			assert(modelMatrixBuffers[i].buffer != nullptr);
			modelMatrixBufferInfo.buffer = modelMatrixBuffers[i].buffer;
			modelMatrixBufferInfo.offset = 0;
			modelMatrixBufferInfo.range = sizeof(glm::mat4) * sceneState.modelMatrices.size();

			VkDescriptorBufferInfo faceColorMatrixBufferInfo{};
			assert(faceColorBuffers[i].buffer != nullptr);
			faceColorMatrixBufferInfo.buffer = faceColorBuffers[i].buffer;
			faceColorMatrixBufferInfo.offset = 0;
			faceColorMatrixBufferInfo.range = faceColorBuffers[i].total_size_bytes;

			VkDescriptorBufferInfo faceIdMapBufferInfo{};
			assert(faceIdMapBuffers[i].buffer != nullptr);
			faceIdMapBufferInfo.buffer = faceIdMapBuffers[i].buffer;
			faceIdMapBufferInfo.offset = 0;
			faceIdMapBufferInfo.range = faceIdMapBuffers[i].total_size_bytes;

			// TODO: This is just bad..
			std::vector<VkWriteDescriptorSet> descriptorWrites;
			if (node.shaderId == ENG_SHADER::PosBB || node.shaderId == ENG_SHADER::PosNorCol)
			{
				descriptorWrites = { 
					createDescriptorWriteUbo(node, i, 0, bufferInfo), 
					createDescriptorWriteModelMatrix(node, i, 1, modelMatrixBufferInfo)
				};
			} 
			else if (node.shaderId == ENG_SHADER::Goldberg)
			{ 
				descriptorWrites = {
					createDescriptorWriteUbo(node, i, 0, bufferInfo),
					createDescriptorWriteModelMatrix(node, i, 1, modelMatrixBufferInfo),
					createDescriptorWriteFaceColorMatrix(node, i, 2, faceColorMatrixBufferInfo),
					createDescriptorWriteFaceIdMapBuffer(node, i, 3, faceIdMapBufferInfo)
				};
			}
			else
			{
				descriptorWrites = {
					createDescriptorWriteUbo(node, i, 0, bufferInfo),
					createDescriptorWriteSampler(node, i, 1, imageInfo),
					createDescriptorWriteModelMatrix(node, i, 2, modelMatrixBufferInfo)
				};
			}

			assert(i < node.descriptorSetIds.size());
			ENG_LOG_INFO("attempt update descriptorsets" << std::endl);

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void createDescriptorSets(ENG::Node& node) {
		if (!node.shaderId.has_value())
		{
			return;
		}
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, pipelineFactory->getDescriptorSetLayout(node.shaderId.value()));
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		node.descriptorSetIds.reserve(MAX_FRAMES_IN_FLIGHT);
		for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			auto new_set = descriptorSets.emplace_back();
			node.descriptorSetIds.push_back(new_set.first);
		}

		assert(node.descriptorSetIds.size() > 0);
		auto &firstSet = descriptorSets.get(node.descriptorSetIds.at(0));
		if (vkAllocateDescriptorSets(device, &allocInfo, &firstSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		writeDescriptorSets(node);
	}

	void createTextureImage() {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(get_tex_path().string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		const ENG::Buffer stagingBuffer(device, physicalDevice, 4, imageSize, 
				  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(device, stagingBuffer.bufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBuffer.bufferMemory);

		stbi_image_free(pixels);

		createImage(
			device, 
			physicalDevice, 
			texWidth, 
			texHeight, 
			VK_FORMAT_R8G8B8A8_SRGB, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			textureImage, 
			textureImageMemory);

		commands->transitionImageLayout(graphicsQueue, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		commands->copyBufferToImage(graphicsQueue, stagingBuffer.buffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		commands->transitionImageLayout(graphicsQueue, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void createTextureImageView() {
		textureImageView = ENG::createImageView(device, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	}


	void createTextureSampler() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	void initGui() {
		//1: create descriptor pool for IMGUI
		// the size of the pool is very oversize, but it's copied from imgui demo itself.
		VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		check_vk_result(vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool));

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		QueueFamilyIndices indices = PhysicalDevice::findQueueFamilies(physicalDevice, surface);

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = instanceFactory->instance;
		init_info.PhysicalDevice = physicalDevice;
		init_info.Device = device;
		init_info.QueueFamily = indices.graphicsFamily.value();
		init_info.Queue = graphicsQueue;
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = imguiPool; // replace
		init_info.RenderPass = renderPass;
		init_info.Subpass = 0;
		init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
		init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info);
	}

	void MySaveFunction()
	{
		ENG_LOG_DEBUG("Save function call" << std::endl);
	}

	void DrawNodeTree(ENG::Node* node) {
		if (ImGui::TreeNode(node->name.c_str())) {
			ImGui::Text("Properties");
			ImGui::Checkbox("Visible", &node->visible);
			ImGui::SliderFloat4("Rotation", &(node->rotation.x), 0.f, 3.1f);
			ImGui::SliderFloat3("Location", &(node->translation.x), 0.f, 3.1f);

			for (const auto& child : node->children) {
				DrawNodeTree(child); // Recursively draw children
			}
			ImGui::TreePop();
		}
	}

	void drawGUI() {
		// Our state
		bool show_demo_window = false;
		bool show_another_window = false;
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
		{
			ImGui::ShowDemoWindow();
		}

		if (sceneState.settings.showSettings)
		{
			ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
			ImGui::Begin("DebugTools");
			ImGui::Text("Camera settings");
			if (ImGui::Button("Save")) MySaveFunction();
			auto& cameraNode = sceneState.graph.nodes.at(sceneState.activeCameraNodeIdx);
			auto* camera = checked_cast<ENG::Component, ENG::Camera>(cameraNode.camera);
			ImGui::SliderFloat("Aspect", &(camera->aspect), 0.0f, 10.0f);
			ImGui::SliderFloat("Fovy", &(camera->fovy), 0.0f, 1.0f);
			ImGui::SliderFloat("zfar", &(camera->zfar), 0.0f, 100.0f);
			ImGui::SliderFloat("znear", &(camera->znear), 0.0f, 10.0f);
			ImGui::InputFloat3("Camera position", &cameraNode.translation.x);
			ImGui::InputFloat3("Camera rotation", &cameraNode.rotation.x);
			
			auto* roomPtr = find_node_by_name(sceneState.graph, "Room");
			ImGui::Text("Room properties");
			ImGui::SliderFloat4("Rotation", &(roomPtr->rotation.x), 0.f, 3.1f);
			ImGui::SliderFloat3("Location", &(roomPtr->translation.x), 0.f, 3.1f);
			auto* suzannePtr = find_node_by_name(sceneState.graph, "Suzanne");
			assert(roomPtr != nullptr);
			assert(suzannePtr != nullptr);
			ImGui::Checkbox("Room visible", &roomPtr->visible);
			ImGui::Checkbox("Suzanne visible", &suzannePtr->visible);
			ImGui::Text("IDX: Name");
			for (auto& node : sceneState.graph.nodes) {
				ImGui::Text("%d: %s", node.nodeId, node.name.c_str());
			}
			DrawNodeTree(sceneState.graph.root);
			ImGui::InputInt("Active: ", &sceneState.activeNodeIdx);
			if (sceneState.activeNodeIdx < sceneState.graph.nodes.size())
			{
				ImGui::SliderFloat4("Active Node Rotation", &(sceneState.graph.nodes.at(sceneState.activeNodeIdx).rotation.x), 0.f, 3.1f);
				ImGui::SliderFloat3("Active Node Location", &(sceneState.graph.nodes.at(sceneState.activeNodeIdx).translation.x), 0.f, 3.1f);
			}
			ImGui::End();
		}

		// Rendering
		ImGui::Render();
	}
};

pmp::Point centroid(const pmp::SurfaceMesh& mesh, pmp::Face f)
{
	pmp::Point c(0, 0, 0);
	pmp::Scalar n(0);
	for (auto v : mesh.vertices(f)) {
		c += mesh.position(v);
		++n;
	}
	c /= n;
	return c;
}

void dual(pmp::SurfaceMesh& mesh)
{
	// the new dual mesh
	pmp::SurfaceMesh tmp;

	// a property to remember new vertices per face
	auto fvertex = mesh.add_face_property<pmp::Vertex>("f:vertex");

	// for each face add the centroid to the dual mesh
	for (auto f : mesh.faces())
	{
		fvertex[f] = tmp.add_vertex(centroid(mesh, f));
	}

	// add new face for each vertex
	for (auto v : mesh.vertices())
	{
		std::vector<pmp::Vertex> vertices;
		for (auto f : mesh.faces(v))
			vertices.push_back(fvertex[f]);

		tmp.add_face(vertices);
	}

	// swap old and new meshes, don't copy properties
	mesh.assign(tmp);
}

void project_to_unit_sphere(pmp::SurfaceMesh& mesh)
{
	for (auto v : mesh.vertices()) {
		auto p = mesh.position(v);
		auto n = norm(p);
		mesh.position(v) = (1.0 / n) * p;
	}
}

pmp::SurfaceMesh create_tetrahedron()
{
	pmp::SurfaceMesh mesh;

	// choose coordinates on the unit sphere
	float a = 1.0f / 3.0f;
	float b = sqrt(8.0f / 9.0f);
	float c = sqrt(2.0f / 9.0f);
	float d = sqrt(2.0f / 3.0f);

	// add the 4 vertices
	auto v0 = mesh.add_vertex(pmp::Point(0, 0, 1));
	auto v1 = mesh.add_vertex(pmp::Point(-c, d, -a));
	auto v2 = mesh.add_vertex(pmp::Point(-c, -d, -a));
	auto v3 = mesh.add_vertex(pmp::Point(b, 0, -a));

	// add the 4 faces
	mesh.add_triangle(v0, v1, v2);
	mesh.add_triangle(v0, v2, v3);
	mesh.add_triangle(v0, v3, v1);
	mesh.add_triangle(v3, v2, v1);

	return mesh;
}

pmp::SurfaceMesh create_hexahedron()
{
	pmp::SurfaceMesh mesh;

	// choose coordinates on the unit sphere
	float a = 1.0f / sqrt(3.0f);

	// add the 8 vertices
	auto v0 = mesh.add_vertex(pmp::Point(-a, -a, -a));
	auto v1 = mesh.add_vertex(pmp::Point(a, -a, -a));
	auto v2 = mesh.add_vertex(pmp::Point(a, a, -a));
	auto v3 = mesh.add_vertex(pmp::Point(-a, a, -a));
	auto v4 = mesh.add_vertex(pmp::Point(-a, -a, a));
	auto v5 = mesh.add_vertex(pmp::Point(a, -a, a));
	auto v6 = mesh.add_vertex(pmp::Point(a, a, a));
	auto v7 = mesh.add_vertex(pmp::Point(-a, a, a));

	// add the 6 faces
	mesh.add_quad(v3, v2, v1, v0);
	mesh.add_quad(v2, v6, v5, v1);
	mesh.add_quad(v5, v6, v7, v4);
	mesh.add_quad(v0, v4, v7, v3);
	mesh.add_quad(v3, v7, v6, v2);
	mesh.add_quad(v1, v5, v4, v0);

	return mesh;
}

pmp::SurfaceMesh create_icosahedron()
{
	pmp::SurfaceMesh mesh;

	float phi = (1.0f + sqrt(5.0f)) * 0.5f; // golden ratio
	float a = 1.0f;
	float b = 1.0f / phi;

	// add vertices
	auto v1 = mesh.add_vertex(pmp::Point(0, b, -a));
	auto v2 = mesh.add_vertex(pmp::Point(b, a, 0));
	auto v3 = mesh.add_vertex(pmp::Point(-b, a, 0));
	auto v4 = mesh.add_vertex(pmp::Point(0, b, a));
	auto v5 = mesh.add_vertex(pmp::Point(0, -b, a));
	auto v6 = mesh.add_vertex(pmp::Point(-a, 0, b));
	auto v7 = mesh.add_vertex(pmp::Point(0, -b, -a));
	auto v8 = mesh.add_vertex(pmp::Point(a, 0, -b));
	auto v9 = mesh.add_vertex(pmp::Point(a, 0, b));
	auto v10 = mesh.add_vertex(pmp::Point(-a, 0, -b));
	auto v11 = mesh.add_vertex(pmp::Point(b, -a, 0));
	auto v12 = mesh.add_vertex(pmp::Point(-b, -a, 0));

	project_to_unit_sphere(mesh);

	// add triangles
	mesh.add_triangle(v3, v2, v1);
	mesh.add_triangle(v2, v3, v4);
	mesh.add_triangle(v6, v5, v4);
	mesh.add_triangle(v5, v9, v4);
	mesh.add_triangle(v8, v7, v1);
	mesh.add_triangle(v7, v10, v1);
	mesh.add_triangle(v12, v11, v5);
	mesh.add_triangle(v11, v12, v7);
	mesh.add_triangle(v10, v6, v3);
	mesh.add_triangle(v6, v10, v12);
	mesh.add_triangle(v9, v8, v2);
	mesh.add_triangle(v8, v9, v11);
	mesh.add_triangle(v3, v6, v4);
	mesh.add_triangle(v9, v2, v4);
	mesh.add_triangle(v10, v3, v1);
	mesh.add_triangle(v2, v8, v1);
	mesh.add_triangle(v12, v10, v7);
	mesh.add_triangle(v8, v11, v7);
	mesh.add_triangle(v6, v12, v5);
	mesh.add_triangle(v11, v9, v5);

	return mesh;
}

pmp::SurfaceMesh create_dodecahedron()
{
	auto mesh = create_icosahedron();
	dual(mesh);
	project_to_unit_sphere(mesh);
	return mesh;
}

ENG::Mesh<VertexPosNorCol>* load_pmp_mesh(const pmp::SurfaceMesh& mesh, const std::string& mesh_name, const std::string& node_name,
	VulkanTemplateApp& app)
{
		std::vector<VertexPosNorCol> vertices;
		std::vector<uint32_t> indices;
		glm::vec3 color{ 0.5f, 0.6f, 0.6f };

		vertices.reserve(mesh.vertices_size());
		indices.resize(12); // unused

		const auto& points = mesh.get_vertex_property<pmp::Point>("v:point");

		for (const auto& face : mesh.faces())
		{
			// extract vertex positions into glm::vec3 positions (counter-clockwise order)
			auto circulator = pmp::SurfaceMesh::VertexAroundFaceCirculator(&mesh, face);
			auto& it = circulator.begin();
			const auto& p0 = points[*it];
			const auto& p1 = points[*(++it)];
			const auto& p2 = points[*(++it)];
			const auto& v0 = glm::vec3(p0[0], p0[1], p0[2]);
			const auto& v1 = glm::vec3(p1[0], p1[1], p1[2]);
			const auto& v2 = glm::vec3(p2[0], p2[1], p2[2]);

			// compute normal vector
			const auto& normal = glm::normalize(glm::cross(v1-v0, v2-v0));

			vertices.emplace_back(v0, normal, color);
			vertices.emplace_back(v1, normal, color);
			vertices.emplace_back(v2, normal, color);
		}

		auto& pmpMesh = app.sceneState.posNorColMeshes.emplace_back(app.device, app.physicalDevice, app.commands.get(), mesh_name, vertices, indices, app.graphicsQueue);
		auto& pmpNode = app.sceneState.graph.nodes.emplace_back();
		pmpNode.name = node_name;
		pmpNode.nodeId = app.sceneState.graph.nodes.size() - 1;
		pmpNode.parent = app.sceneState.graph.root;
		pmpNode.mesh = &pmpMesh;
		pmpNode.shaderId = ENG_SHADER::Goldberg;
		app.sceneState.graph.root->children.push_back(&pmpNode);

		return &pmpMesh;
}

void triangulate_as_triangle_fan_preserving_face_ids(pmp::SurfaceMesh& mesh)
{
	// the new dual mesh
	pmp::SurfaceMesh tmp;

	// apply a face index for each face (preserved after triangulation)
	auto oldFaceId = mesh.get_face_property<uint32_t>("f:faceId");
	auto newFaceId = tmp.add_face_property<uint32_t>("f:faceId");

	auto facecount{ 0 };
	for (auto f : mesh.faces())
	{
		// calculate center of triangle fan for new mesh face
		const auto& centerVert = tmp.add_vertex(centroid(mesh, f));

		// iterate over all vertices in original face, creating triangles
		auto vertRange = mesh.vertices(f);
		auto it = vertRange.begin();
		const auto& end = vertRange.end();
		ENG_LOG_DEBUG("Verts in face: " << std::distance(it, end) << std::endl);
		assert(std::distance(it, end) > 2);
		auto v0 = tmp.add_vertex(mesh.position(*(it++)));
		auto firstVert = v0;
		auto v1 = tmp.add_vertex(mesh.position(*(it++)));
		auto tri = tmp.add_triangle(centerVert, v0, v1);
		facecount++;
		newFaceId[tri] = oldFaceId[f];

		for (; it != end; ++it)
		{
			v0 = v1;
			v1 = tmp.add_vertex(mesh.position(*(it)));
			tri = tmp.add_triangle(centerVert, v0, v1);
			newFaceId[tri] = oldFaceId[f];
			facecount++;
			ENG_LOG_DEBUG("INNER LOOP HIT" << std::endl);
		}

		// add last triangle using last and first vertex
		tri = tmp.add_triangle(centerVert, v1, firstVert);
		newFaceId[tri] = oldFaceId[f];
		facecount++;
	}

	ENG_LOG_DEBUG("FAce count " << facecount << std::endl);
	// deep copy of tmp mesh, including properties	
	mesh = tmp;
}

int main() {
	
	try {
		printf("Starting app\n");
		// ENG_LOG_INFO("Application path: " << install_dir.native().c_str() << std::endl);

		VulkanTemplateApp app;
		ENG_LOG_INFO(app);

		// TODO: implement pools to avoid reference invalidation on reallocation problem
		app.sceneState.posColTexMeshes.reserve(100);
		app.sceneState.posNorTexMeshes.reserve(100);
		app.sceneState.posMeshes.reserve(100);
		app.sceneState.posNorColMeshes.reserve(100);
		app.sceneState.graph.nodes.reserve(100);
		app.sceneState.graph.cameras.reserve(10);

		auto& attachmentPoint = app.sceneState.graph.nodes.emplace_back();
		app.sceneState.graph.root = &attachmentPoint;
		app.sceneState.graph.root->name = "Root";

		// ENG_LOG_INFO("GLTF path: " << gltf_dir.native().c_str() << std::endl);
		load_gltf(app.device, app.physicalDevice, app.graphicsQueue, app.commands.get(), get_gltf_dir(), app.sceneState, attachmentPoint);
		auto& cameraNode = app.sceneState.graph.nodes.at(app.sceneState.activeCameraNodeIdx);
		cameraNode.translation.x = 2.f;
		cameraNode.translation.y = 2.f;
		cameraNode.translation.z = 2.f;

		const auto& meshName = std::string("Room");
		ENG::loadModel(app.device, app.physicalDevice, app.commands.get(), meshName, app.graphicsQueue, get_model_dir(), app.sceneState, attachmentPoint);

		// Create bounding box around Suzanne
		auto* suzanneNode = find_node_by_name(app.sceneState.graph, "Suzanne");
		if (suzanneNode != nullptr)
		{
			const auto* mesh = checked_cast<ENG::Component, ENG::Mesh<VertexPosNorTex>>(suzanneNode->mesh);
			const auto& minXIt = std::min_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
				return (rhs.pos.x < lhs.pos.x);
			});
			const auto& minYIt = std::min_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
				return (rhs.pos.y < lhs.pos.y);
			});
			const auto& minZIt = std::min_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
				return (rhs.pos.z < lhs.pos.z);
			});
			const auto& maxXIt = std::max_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
				return (rhs.pos.x < lhs.pos.x);
			});
			const auto& maxYIt = std::max_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
				return (rhs.pos.y < lhs.pos.y);
			});
			const auto& maxZIt = std::max_element(mesh->vertices.begin(), mesh->vertices.end(), [](const VertexPosNorTex& rhs, const VertexPosNorTex& lhs) {
				return (rhs.pos.z < lhs.pos.z);
			});

			const auto& maxX = maxXIt->pos.x;
			const auto& maxY = maxYIt->pos.y;
			const auto& maxZ = maxZIt->pos.z;
			const auto& minX = minXIt->pos.x;
			const auto& minY = minYIt->pos.y;
			const auto& minZ = minZIt->pos.z;

			std::vector<VertexPos> bbVertices {
				{ { minX, minY, maxZ } },
				{ { maxX, minY, maxZ } },
				{ { maxX, maxY, maxZ } },
				{ { minX, maxY, maxZ } },
				{ { minX, minY, minZ } },
				{ { maxX, minY, minZ } },
				{ { maxX, maxY, minZ } },
				{ { minX, maxY, minZ } },
			};

			std::vector<uint32_t> bbIndices {
				0, 1, 1, 2, 2, 3, 3, 0,
				4, 5, 5, 6, 6, 7, 7, 4,
				0, 4, 1, 5, 2, 6, 3, 7
			};

			auto& bbMesh = app.sceneState.posMeshes.emplace_back(app.device, app.physicalDevice, app.commands.get(), "SuzanneBB" , bbVertices, bbIndices, app.graphicsQueue);
			auto& bbNode = app.sceneState.graph.nodes.emplace_back();
			bbNode.name = "SuzanneBoundingBox";
			bbNode.nodeId = app.sceneState.graph.nodes.size() - 1;
			bbNode.parent = suzanneNode;
			bbNode.mesh = &bbMesh;
			bbNode.shaderId = ENG_SHADER::PosBB;
			suzanneNode->children.push_back(&bbNode);
		}

		// Create Tetrahedron
		{
			std::vector<VertexPosNorCol> tetraVertices {
				{ {1.,  1.,  1.} },
				{ {1., -1., -1.} },
				{ {-1., 1., -1.} },
				{ {-1., -1., 1.} }
			};

			std::vector<uint32_t> tetraIndices {
				0, 1, 2,
				0, 3, 1,
				0, 2, 3,
				3, 2, 1
			};

			std::vector<glm::vec3> colors {
				{1.0, 0.5, 0.5},
				{0.5, 1.0, 0.5},
				{0.5, 0.5, 1.0},
				{0.5, 0.5, 0.5},
			};

			// vertices are duplicated for face-specific color
			std::vector<VertexPosNorCol> tetraVerticesDuplicated {
				tetraVertices.at(0), tetraVertices.at(1), tetraVertices.at(2),
				tetraVertices.at(0), tetraVertices.at(3), tetraVertices.at(1),
				tetraVertices.at(0), tetraVertices.at(2), tetraVertices.at(3),
				tetraVertices.at(3), tetraVertices.at(2), tetraVertices.at(1)
			};

			for (size_t i = 0; i < 4; ++i)
			{
				auto normal = glm::normalize(
					glm::cross(
						tetraVerticesDuplicated.at(i * 3 + 1).pos - tetraVerticesDuplicated.at(i * 3).pos,
						tetraVerticesDuplicated.at(i * 3 + 2).pos - tetraVerticesDuplicated.at(i * 3).pos
					)
				);
				for (size_t j = 0; j < 3; ++j)
				{
					tetraVerticesDuplicated.at(i * 3 + j).normal = normal;
					tetraVerticesDuplicated.at(i * 3 + j).color = colors.at(i);
				}
			}


			auto& tetraMesh = app.sceneState.posNorColMeshes.emplace_back(app.device, app.physicalDevice, app.commands.get(), "TetrahedronMesh", tetraVerticesDuplicated, tetraIndices, app.graphicsQueue);
			auto& tetraNode = app.sceneState.graph.nodes.emplace_back();
			tetraNode.name = "Tetrahedron";
			tetraNode.nodeId = app.sceneState.graph.nodes.size() - 1;
			tetraNode.parent = app.sceneState.graph.root;
			tetraNode.mesh = &tetraMesh;
			tetraNode.shaderId = ENG_SHADER::PosNorCol;
			app.sceneState.graph.root->children.push_back(&tetraNode);
		}

		// Create triangulated goldberg polyhedra including faceIds
		{
			auto mesh = pmp::icosphere(2);
			dual(mesh);

			// apply a face index for each face (preserved after triangulation)
			auto faceId = mesh.add_face_property<uint32_t>("f:faceId");

			auto facecount = static_cast<uint32_t>(0);
			for (auto f : mesh.faces())
			{
				faceId[f] = facecount++;
				ENG_LOG_DEBUG("PRE TRIANGULARIZATION FACEID: " << faceId[f] << std::endl);
			}

			// Vector of face colors indexed by faceId
			std::vector<glm::vec4> faceColors;
			faceColors.resize(facecount);

			for (auto i = 0; i < facecount; ++i)
			{
				faceColors[i] = glm::vec4(0.4f, 0.4f, 0.7f, 1.0f);
			}

			const auto& oceanblue = glm::vec4(0.f, 0.369f, 0.722f, 1.0f);
			const auto& forestgreen = glm::vec4(0.133f, 0.545f, 0.133f, 1.f);
			const auto& sunset = glm::vec4(0.98f, 0.77f, 0.4f, 1.f);
			
			// First 12 faces are pentagons
			auto count = 0;
			for (auto f : mesh.faces())
			{
				if (count++ < 12)
				{
					faceColors[f.idx()] = forestgreen;

					// Circulate around first outer ring of pentagon faces and paint faces
					for (auto h : mesh.halfedges(f))
					{
						auto h2 = mesh.opposite_halfedge(h);
						auto f2 = mesh.face(h2);
						faceColors[f2.idx()] = forestgreen;

						// Jump 2 half-edges of face on outer ring, and paint
						h2 = mesh.next_halfedge(mesh.next_halfedge(h2));
						auto h3 = mesh.opposite_halfedge(h2);
						auto f3 = mesh.face(h3);
						faceColors[f3.idx()] = oceanblue;

						// Jump 3rd half-edge of face on outer ring and paint
						h2 = mesh.next_halfedge(h2);
						h3 = mesh.opposite_halfedge(h2);
						f3 = mesh.face(h3);
						faceColors[f3.idx()] = oceanblue;
					}

				}
			}
			app.createFaceColorBuffers(facecount);
			const auto& colorBufferSize = faceColors.size() * sizeof(glm::vec4);
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				memcpy(app.faceColorBuffersMapped[i], faceColors.data(), colorBufferSize);
			}

			triangulate_as_triangle_fan_preserving_face_ids(mesh);

			// Vector of faceIds indexed by primitiveId (gl_PrimitiveID) to lookup faceID in shader
			std::vector<uint32_t> primitiveToFaceIdMap;
			primitiveToFaceIdMap.reserve(mesh.faces_size());
			faceId = mesh.get_face_property<uint32_t>("f:faceId");
			for (auto f : mesh.faces())
			{
				primitiveToFaceIdMap.push_back(faceId[f]);
				ENG_LOG_DEBUG("POST TRIANGULURIZATION FACEID: " << faceId[f] << std::endl);
			}
			load_pmp_mesh(mesh, "GoldbergMesh", "GoldbergPolyhedra", app);

			app.createFaceIdBuffers(mesh.faces_size());
			const auto& bufferSize = primitiveToFaceIdMap.size() * sizeof(uint32_t);
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				memcpy(app.faceIdMapBuffersMapped[i], primitiveToFaceIdMap.data(), bufferSize);
			}
		}

		// Create modelMatrices mapped to SceneGraph node idx (for now, 1-1 with scenegraph.nodes)
		app.sceneState.modelMatrices.resize(app.sceneState.graph.nodes.size());

		app.createModelMatrices();

		for (auto& node : app.sceneState.graph.nodes)
		{
			app.createDescriptorSets(node);
		}

		ENG_LOG_INFO("PosColTex Meshes loaded:" << std::endl);
		for (const auto& mesh : app.sceneState.posColTexMeshes)
		{
			ENG_LOG_INFO("\t" << mesh.name << std::endl);
		}

		ENG_LOG_INFO("PosNorTex Meshes loaded:" << std::endl);
		for (const auto& mesh : app.sceneState.posNorTexMeshes)
		{
			ENG_LOG_INFO("\t" << mesh.name << std::endl);
		}

		ENG_LOG_INFO("PosBB Meshes loaded:" << std::endl);
		for (const auto& mesh : app.sceneState.posMeshes)
		{
			ENG_LOG_INFO("\t" << mesh.name << std::endl);
		}

		ENG_LOG_INFO("Finished loading data" << std::endl);

		ENG_LOG_DEBUG("Size of NODE (bytes): " << sizeof(ENG::Node) << std::endl);

		// custom settings overrides
		if (suzanneNode != nullptr)
		{
			suzanneNode->visible = false;
		}
		auto* suzanneBoundingBoxNode = find_node_by_name(app.sceneState.graph, "SuzanneBoundingBox");
		if (suzanneBoundingBoxNode != nullptr)
		{
			suzanneBoundingBoxNode->visible = false;
		}
		auto* roomNode = find_node_by_name(app.sceneState.graph, "Room");
		if (roomNode != nullptr)
		{
			roomNode->visible = false;
		}
		auto* tetrahedronNode = find_node_by_name(app.sceneState.graph, "Tetrahedron");
		if (tetrahedronNode != nullptr)
		{
			tetrahedronNode->visible = false;
		}
		auto* camera = checked_cast<ENG::Component, ENG::Camera>(cameraNode.camera);
		camera->fovy = 0.7;

		app.run();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	    	return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
