#include<iostream>
#include<locale>
#include<vector>
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
#include "EngineConfig.h"

#include<tiny_gltf.h>
#include<tiny_obj_loader.h>
#include<stb_image.h>

#ifdef _WIN32
#include "tracy/Tracy.hpp"
#endif

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "primitives/mesh.hpp"
#include "pipelines/shader_factory.hpp"
#include "pipelines/pipeline_factory.hpp"
#include "interfaces/FilesystemInterface.hpp"
#include "interfaces/command.h"
#include "interfaces/swapchain.h"
#include "interfaces/device.h"
#include "interfaces/PhysicalDevice.h"
#include "interfaces/image.h"
#include "interfaces/obj.h"
#include "interfaces/scene.h"
#include "interfaces/gltf.h"
#include "interfaces/Instance.h"
#include "interfaces/buffer.h"
#include "interfaces/logging.h"
#include "interfaces/gui.h"


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
		sceneState.posColTexMeshes.clear();
		sceneState.posNorTexMeshes.clear();

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

			//auto& camera_rotation = sceneState->scene.nodes[sceneState->activeCameraNodeIdx].rotation;
			//auto cam_quat = glm::quat(camera_rotation[3], camera_rotation[0], camera_rotation[1], camera_rotation[2]);

			constexpr float sensitivity = 0.1f;
			// const auto& local_x_axis = glm::normalize(sceneState->test_model[0]);
			// auto dx_radians = glm::angleAxis(glm::radians(static_cast<float>(dx) * sensitivity), glm::vec3(0.0f, 0.0f, 1.0f));
			// auto dy_radians = glm::angleAxis(glm::radians(static_cast<float>(dy) * sensitivity), glm::vec3(local_x_axis.x, local_x_axis.y, local_x_axis.z));
			//cam_quat = cam_quat * dx_radians;
			//camera_rotation[0] = cam_quat.z;
			//camera_rotation[1] = cam_quat.x;
			//camera_rotation[2] = cam_quat.y;
			//camera_rotation[3] = cam_quat.w;

			// auto& test_rot = sceneState->test_model;
			// rotate 3 degrees around y-axis when E is pressed
			//auto dx = glm::angleAxis(glm::radians(3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			// test_rot = glm::mat4_cast(dy_radians) * glm::mat4_cast(dx_radians) * test_rot;
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
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineFactory->getVkPipeline(node.shaderId.value()));

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineFactory->getVkPipelineLayout(node.shaderId.value()),
				  0, 1, &descriptorSets.get(node.descriptorSetIds.at(currentFrame)), 0, nullptr);


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
			uniformBuffers.emplace_back(device, physicalDevice, bufferSize,
			       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			vkMapMemory(device, uniformBuffers[i].bufferMemory, 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
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
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
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

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i].buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView;
			imageInfo.sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			assert(i < node.descriptorSetIds.size());
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets.get(node.descriptorSetIds.at(i));
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets.get(node.descriptorSetIds.at(i));;
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;
			ENG_LOG_INFO("attempt update descriptorsets" << std::endl);

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void createTextureImage() {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(get_tex_path().string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		const ENG::Buffer stagingBuffer(device, physicalDevice, imageSize, 
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
			ImGui::Text("Camera settings");
			if (ImGui::Button("Save")) MySaveFunction();
			auto& cameraNode = sceneState.graph.nodes.at(sceneState.activeCameraNodeIdx);
			auto* camera = CAST_OR_DIE(dynamic_cast<ENG::Camera*>(cameraNode.camera));
			ImGui::SliderFloat("Aspect", &(camera->aspect), 0.0f, 10.0f);
			ImGui::SliderFloat("Fovy", &(camera->fovy), 0.0f, 1.0f);
			ImGui::SliderFloat("zfar", &(camera->zfar), 0.0f, 100.0f);
			ImGui::SliderFloat("znear", &(camera->znear), 0.0f, 10.0f);
			ImGui::InputFloat3("Camera position", &cameraNode.translation.x);
			ImGui::InputFloat3("Camera rotation", &cameraNode.rotation.x);
			
			auto* roomPtr = find_node_by_name(sceneState.graph, "Room");
			auto* suzannePtr = find_node_by_name(sceneState.graph, "Suzanne");
			assert(roomPtr != nullptr);
			assert(suzannePtr != nullptr);
			ImGui::Checkbox("Room visible", &roomPtr->visible);
			ImGui::Checkbox("Suzanne visible", &suzannePtr->visible);
		}

		// Rendering
		ImGui::Render();
	}
};

int main() {
	
	try {
		printf("Starting app\n");
		// ENG_LOG_INFO("Application path: " << install_dir.native().c_str() << std::endl);

		VulkanTemplateApp app;
		ENG_LOG_INFO(app);

		// TODO: implement pools to avoid reference invalidation on reallocation problem
		app.sceneState.posColTexMeshes.reserve(10);
		app.sceneState.posNorTexMeshes.reserve(10);
		app.sceneState.graph.nodes.reserve(10);
		app.sceneState.graph.cameras.reserve(10);

		auto& attachmentPoint = app.sceneState.graph.nodes.emplace_back();
		app.sceneState.graph.root = &attachmentPoint;

		// ENG_LOG_INFO("GLTF path: " << gltf_dir.native().c_str() << std::endl);
		load_gltf(app.device, app.physicalDevice, app.graphicsQueue, app.commands.get(), get_gltf_dir(), app.sceneState, attachmentPoint);
		const auto& meshName = std::string("Room");
		ENG::loadModel(app.device, app.physicalDevice, app.commands.get(), meshName, app.graphicsQueue, get_model_dir(), app.sceneState, attachmentPoint);

		for (auto* node : app.sceneState.graph.root->children)
		{
			assert(node != nullptr);
			app.createDescriptorSets(*node);
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

		ENG_LOG_INFO("Finished loading data" << std::endl);

		ENG_LOG_DEBUG("Size of NODE (bytes): " << sizeof(ENG::Node) << std::endl);

		app.run();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	    	return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
