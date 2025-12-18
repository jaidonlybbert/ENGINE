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
#include<random>

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

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"


#include "EngineConfig.hpp"
#include "renderer/Utils.hpp"
#include "scene/Mesh.hpp"
#include "renderer/pipelines/ShaderFactory.hpp"
#include "renderer/pipelines/PipelineFactory.hpp"
#include "filesystem/FilesystemInterface.hpp"
#include "renderer/Command.hpp"
#include "renderer/Swapchain.hpp"
#include "renderer/Device.hpp"
#include "renderer/PhysicalDevice.hpp"
#include "renderer/Image.hpp"
#include "scene/Obj.hpp"
#include "scene/Scene.hpp"
#include "scene/Gltf.hpp"
#include "renderer/Instance.hpp"
#include "renderer/Buffer.hpp"
#include "logger/Logging.hpp"
#include "gui/Gui.hpp"
#include "renderer/Renderer.hpp"
#include "scene/DFT.hpp"

using namespace ENG;

VulkanTemplateApp::VulkanTemplateApp() {
}

void VulkanTemplateApp::registerInitializationFunction(std::function<void(void)> initFunc) {
	initializationFunctions.push_back(initFunc);
}

void VulkanTemplateApp::initialize() {
	for (auto& initFunc : initializationFunctions) {
		initFunc();
	}
}


void VulkanTemplateApp::run() {
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
		ENG_LOG_TRACE("process lua" << std::endl);
#ifdef _WIN32
		FrameMarkEnd("run_frame");
#endif

	}

	vkDeviceWaitIdle(device);
}

void VulkanTemplateApp::initializeScene(std::function<void(VulkanTemplateApp&)> loadFunction) {
	std::lock_guard<std::mutex> lock(scene_mtx);
	sceneReadyToRender = false;
	loadFunction(*this);
	sceneReadyToRender = true;
}

VulkanTemplateApp::~VulkanTemplateApp() {
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


void VulkanTemplateApp::cleanupGui() {
	vkDestroyDescriptorPool(device, imguiPool, nullptr);
}

std::ostream& operator<<(std::ostream& os, VulkanTemplateApp& app) {
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

void VulkanTemplateApp::framebufferResizeCallback(GLFWwindow* window, int width, int height) 
{
	auto app = reinterpret_cast<VulkanTemplateApp*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

void VulkanTemplateApp::initVulkan() 
{
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

VkShaderModule VulkanTemplateApp::createShaderModule(const std::vector<char>& code) {
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

void VulkanTemplateApp::createSurface() {
	if (glfwCreateWindowSurface(instanceFactory->instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void VulkanTemplateApp::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
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

	if (sceneReadyToRender) {
		for (auto& commandRecorder : commandRecorders) {
			commandRecorder(commandBuffer);
		}
	}

	// ENG::ImplVulkan_RenderDrawData(ENG::GetDrawData(), commandBuffer);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRenderPass(commandBuffer);
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void VulkanTemplateApp::registerCommandRecorder(std::function<void(VkCommandBuffer)> commandRecorder) {
	commandRecorders.push_back(commandRecorder);
}

void VulkanTemplateApp::drawFrame() 
{
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

	if (sceneReadyToRender) {
		updateUniformBuffer(currentFrame);
		updateModelMatrixBuffer();
	}

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

void VulkanTemplateApp::createSyncObjects() 
{
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

void VulkanTemplateApp::createUniformBuffers() 
{
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
void VulkanTemplateApp::createModelMatrices()
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

void VulkanTemplateApp::createFaceIdBuffers(const uint32_t number_of_faces)
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

void VulkanTemplateApp::createFaceColorBuffers(const uint32_t number_of_faces)
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

void VulkanTemplateApp::updateModelMatrix(glm::mat4& modelMatrix, const ENG::Node& node)
{
	// Compute local transform from TRS data
	modelMatrix = glm::translate(glm::mat4(1.f), node.translation)
		* glm::mat4_cast(node.rotation)
		* glm::scale(glm::mat4(1.f), node.scale);
}


void VulkanTemplateApp::updateModelMatrixBuffer()
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

void VulkanTemplateApp::updateUniformBuffer(uint32_t currentImage) 
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

	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void VulkanTemplateApp::createDescriptorPool() 
{
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

VkWriteDescriptorSet VulkanTemplateApp::createDescriptorWriteModelMatrix(
	const ENG::Node& node, 
	const size_t frameIdx, 
	const size_t bindingIdx, 
	const VkDescriptorBufferInfo& modelMatrixBufferInfo) 
{
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

VkWriteDescriptorSet VulkanTemplateApp::createDescriptorWriteSampler(
	const ENG::Node& node, 
	const size_t frameIdx, 
	const size_t bindingIdx, 
	const VkDescriptorImageInfo& imageInfo) 
{
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

VkWriteDescriptorSet VulkanTemplateApp::createDescriptorWriteUbo(
	const ENG::Node& node, 
	const size_t frameIdx, 
	const size_t bindingIdx, 
	const VkDescriptorBufferInfo& bufferInfo) 
{
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

VkWriteDescriptorSet VulkanTemplateApp::createDescriptorWriteFaceColorMatrix(
	const ENG::Node& node, 
	const size_t frameIdx, 
	const size_t bindingIdx, 
	const VkDescriptorBufferInfo& bufferInfo)
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

VkWriteDescriptorSet VulkanTemplateApp::createDescriptorWriteFaceIdMapBuffer(
	const ENG::Node& node, 
	const size_t frameIdx, 
	const size_t bindingIdx, 
	const VkDescriptorBufferInfo& bufferInfo)
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

void VulkanTemplateApp::writeDescriptorSets(const ENG::Node& node) 
{
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
		if (node.shaderId == "PosBB" || node.shaderId == "PosNorCol")
		{
			descriptorWrites = { 
				createDescriptorWriteUbo(node, i, 0, bufferInfo), 
				createDescriptorWriteModelMatrix(node, i, 1, modelMatrixBufferInfo)
			};
		} 
		else if (node.shaderId == "Goldberg")
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

void VulkanTemplateApp::createDescriptorSets(ENG::Node& node) 
{
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

void VulkanTemplateApp::createTextureImage() 
{
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

void VulkanTemplateApp::createTextureImageView() 
{
	textureImageView = ENG::createImageView(device, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanTemplateApp::createTextureSampler() 
{
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

void VulkanTemplateApp::initGui() 
{
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

void VulkanTemplateApp::MySaveFunction()
{
	ENG_LOG_DEBUG("Save function call" << std::endl);
}

void VulkanTemplateApp::DrawNodeTree(ENG::Node* node) 
{
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

void VulkanTemplateApp::drawGUI() 
{
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
