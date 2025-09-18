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
#include "vulkan/vulkan.hpp"
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

using namespace ENG;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

class Camera : Component {
public:
	Camera(const tinygltf::Camera& camera) {
		fovy = static_cast<float>(camera.perspective.yfov);
		aspect = static_cast<float>(camera.perspective.aspectRatio);
		znear = static_cast<float>(camera.perspective.znear);
		zfar = static_cast<float>(camera.perspective.zfar);
	}
	float fovy, aspect, znear, zfar;
};


struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
		const VkAllocationCallbacks* pAllocator, 
		VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, 
			"vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
		VkDebugUtilsMessengerEXT debugMessenger, 
		const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
			"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
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
			glfwPollEvents();
			drawGUI();
			drawFrame();
#ifdef _WIN32
			FrameMarkEnd("run_frame");
#endif

		}

		vkDeviceWaitIdle(device);
	}

	~VulkanTemplateApp() {

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		cleanupGui();

		swapchain->cleanupSwapChain(device);

		vkDestroySampler(device, textureSampler, nullptr);
		vkDestroyImageView(device, textureImageView, nullptr);
		vkDestroyImage(device, textureImage, nullptr);
		vkFreeMemory(device, textureImageMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		for (auto& buffer : indexBuffer) {
			vkDestroyBuffer(device, buffer, nullptr);
		}

		for (auto& bufferMemory : indexBufferMemory) {
			vkFreeMemory(device, bufferMemory, nullptr);
		}

		for (auto& buffer : vertexBuffer) {
			vkDestroyBuffer(device, buffer, nullptr);
		}

		for (auto& bufferMemory : vertexBufferMemory) {
			vkFreeMemory(device, bufferMemory, nullptr);
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commands->commandPool, nullptr);

		// Calls destructors for pipelines/shaders
		pipelineFactory.reset();

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}


	void cleanupGui() {
		vkDestroyDescriptorPool(device, imguiPool, nullptr);
	}

	friend std::ostream& operator<<(std::ostream& os, VulkanTemplateApp& app) {
		// Print application name and version
		std::cout << PROJECT_NAME_AND_VERSION << std::endl;
		// Print physical device properties
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(app.physicalDevice, &deviceProperties);
		std::cout << std::endl << "Physical Device Properties: " << std::endl;
		std::cout << "Name:\t" << deviceProperties.deviceName << std::endl;
		std::cout << "API Version:\t" << deviceProperties.apiVersion << std::endl;
		std::cout << "Driver Version:\t" << deviceProperties.driverVersion << std::endl;
		std::cout << std::endl;
		
		// Print available extensions
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
		std::cout << "Available Vulkan Extensions:" << std::endl;
		for (const auto& extension: availableExtensions) {
			std::cout << '\t' << extension.extensionName << std::endl;
		}

		// Print used extensions
		auto enabledExtensions = app.getRequiredExtensions();

		std::cout << "Enabled Vulkan Extensions:" << std::endl;
		for (auto extension : enabledExtensions) {
			std::cout << '\t' << extension << std::endl;
		}
		return os;
	}


	void loadMeshesToVkBuffer(SceneState& sceneState) {
		ENG::loadModel(sceneState);

		if (sceneState.posColTexMeshes.size() > 0)
		{
			std::cout << "Loading VkBuffers for posColTex meshes of size " << sceneState.posColTexMeshes.size() << std::endl;
			auto& posColTexVertexVkBuffer = vertexBuffer.emplace_back();
			auto& posColTexVertexVkBufferMemory = vertexBufferMemory.emplace_back();
			auto& posColTexIndexVkBuffer = indexBuffer.emplace_back();
			auto& posColTexIndexVkBufferMemory = indexBufferMemory.emplace_back();
			createVertexBuffer(sceneState.posColTexMeshes, posColTexVertexVkBuffer, posColTexVertexVkBufferMemory);
			createIndexBuffer(sceneState.posColTexMeshes, posColTexIndexVkBuffer, posColTexIndexVkBufferMemory);
		}

		if (sceneState.posNorTexMeshes.size() > 0)
		{
			std::cout << "Creating VkBuffers for posNorTex meshes of size " << sceneState.posColTexMeshes.size() << std::endl;
			auto& posNorTexVertexVkBuffer = vertexBuffer.emplace_back();
			auto& posNorTexVertexVkBufferMemory = vertexBufferMemory.emplace_back();
			auto& posNorTexIndexVkBuffer = indexBuffer.emplace_back();
			auto& posNorTexIndexVkBufferMemory = indexBufferMemory.emplace_back();
			createVertexBuffer(sceneState.posNorTexMeshes, posNorTexVertexVkBuffer, posNorTexVertexVkBufferMemory);
			createIndexBuffer(sceneState.posNorTexMeshes, posNorTexIndexVkBuffer, posNorTexIndexVkBufferMemory);
		}
	}

	SceneState sceneState;
private:
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	std::vector<VkPipeline> graphicsPipelines;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	bool framebufferResized = false;
	uint32_t currentFrame = 0;
	std::vector<VkBuffer> vertexBuffer;
	std::vector<VkDeviceMemory> vertexBufferMemory;
	std::vector<VkBuffer> indexBuffer;
	std::vector<VkDeviceMemory> indexBufferMemory;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	VkDescriptorPool descriptorPool;
	VkDescriptorPool imguiPool;
	std::vector<VkDescriptorSet> descriptorSets;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	std::unique_ptr<ENG::PipelineFactory> pipelineFactory;
	std::unique_ptr<ENG::Command> commands;
	std::unique_ptr<ENG::Swapchain> swapchain;


	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_E && action == GLFW_PRESS)
		{
			std::cout << "E key down" << std::endl;
			auto* sceneState = static_cast<SceneState*>(glfwGetWindowUserPointer(window));
			auto& camera_rotation = sceneState->scene.nodes[sceneState->activeCameraNodeIdx].rotation;
			auto cam_quat = glm::quat(camera_rotation[3], camera_rotation[0], camera_rotation[1], camera_rotation[2]);
			// rotate 3 degrees around x-axis when E is pressed
			auto dx = glm::angleAxis(glm::radians(3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			cam_quat = cam_quat * dx;
			camera_rotation[0] = cam_quat.z;
			camera_rotation[1] = cam_quat.x;
			camera_rotation[2] = cam_quat.y;
			camera_rotation[3] = cam_quat.w;
		}

		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			std::cout << "R key down" << std::endl;
			auto* sceneState = static_cast<SceneState*>(glfwGetWindowUserPointer(window));
			auto& test_rot = sceneState->test_model;
			// rotate 3 degrees around y-axis when E is pressed
			auto dx = glm::angleAxis(glm::radians(3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			test_rot = glm::mat4_cast(dx) * test_rot;
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
				std::cout << "Middle mouse initial press" << std::endl;
			}
			else // action is GLFW_RELEASE
			{
				std::cout << "Middle mouse released" << std::endl;
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
			std::cout << "Shift down" << std::endl;
			auto& camera = sceneState->scene.cameras[sceneState->scene.nodes[sceneState->activeCameraNodeIdx].camera];
			auto& fovy = camera.perspective.yfov;
			fovy += 0.1 * yoffset;
			if (static_cast<float>(fovy) < 0.1) fovy = 0.1; // Prevent zooming too far out
			printf("FOV: %.2f\n", fovy);
		}
		else
		{
			auto& camera_rotation = sceneState->scene.nodes[sceneState->activeCameraNodeIdx].rotation;
			auto& camera_position = sceneState->scene.nodes[sceneState->activeCameraNodeIdx].translation;
			assert(camera_position.size() == 3);
			auto cam_quat = glm::quat(camera_rotation[3], camera_rotation[0], camera_rotation[1], camera_rotation[2]);

			// Default forward vector in world space
			glm::vec3 forward(0.0f, 0.0f, -1.0f);

			// Rotate the forward vector by the quaternion
			glm::vec3 rotatedForward = cam_quat * forward;

			// Output the result
			std::cout << "Forward vector: ("
				<< rotatedForward.x << ", "
				<< rotatedForward.y << ", "
				<< rotatedForward.z << ")" << std::endl;

			// Move camera forward or backward
			camera_position[0] += rotatedForward[0] * 0.1f;
			camera_position[1] += rotatedForward[1] * 0.1f;
			camera_position[2] += rotatedForward[2] * 0.1f;
			std::cout << "Camera position: ("
				<< camera_position[0] << ", "
				<< camera_position[1] << ", "
				<< camera_position[2] << ")" << std::endl;
		}
	}


	static void mouse_movement_callback(GLFWwindow* window, double xpos, double ypos)
	{
		static double dx, dy = 0.f;
		auto* sceneState = static_cast<SceneState*>(glfwGetWindowUserPointer(window));
		const auto& middle_mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);
		if (middle_mouse_state == GLFW_PRESS)
		{
			std::cout << "Middle mouse down" << std::endl;
			dx += xpos - sceneState->cursor_x;
			dy += ypos - sceneState->cursor_y;
			sceneState->cursor_x = xpos;
			sceneState->cursor_y = ypos;

			auto& camera_rotation = sceneState->scene.nodes[sceneState->activeCameraNodeIdx].rotation;
			auto cam_quat = glm::quat(camera_rotation[3], camera_rotation[0], camera_rotation[1], camera_rotation[2]);
			// rotate 3 degrees around x-axis when E is pressed
			constexpr float sensitivity = 0.01f;
			auto dx_radians = glm::angleAxis(glm::radians(static_cast<float>(dx) * sensitivity), glm::vec3(0.0f, 1.0f, 0.0f));
			auto dy_radians = glm::angleAxis(glm::radians(static_cast<float>(dy) * sensitivity), glm::vec3(1.0f, 0.0f, 0.0f));
			cam_quat = cam_quat * dx_radians;
			camera_rotation[0] = cam_quat.z;
			camera_rotation[1] = cam_quat.x;
			camera_rotation[2] = cam_quat.y;
			camera_rotation[3] = cam_quat.w;
		}
		else
		{
			dx = 0.f, dy = 0.f;
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
		createInstance();
		setupDebugMessenger();
		createSurface();
		ENG::PhysicalDevice::pickPhysicalDevice(instance, physicalDevice, surface);
		ENG::Device::createLogicalDevice(surface, physicalDevice, validationLayers, graphicsQueue, presentQueue, device);
		swapchain = std::make_unique<Swapchain>(physicalDevice, surface, device, *window);
		pipelineFactory = std::make_unique<ENG::PipelineFactory>(device, swapchain->swapChainImageFormat, findDepthFormat(physicalDevice));
		renderPass = pipelineFactory->getRenderPass();
		descriptorSetLayout = pipelineFactory->getDescriptorSetLayout(ENG_SHADER::PosColTex);
		graphicsPipelines = pipelineFactory->getVkPipelines();
		pipelineLayout = pipelineFactory->getVkPipelineLayout(ENG_SHADER::PosColTex);
		commands = std::make_unique<Command>(physicalDevice, device, surface); // creates command pool
		createDepthResources(device, physicalDevice, swapchain->swapChainExtent, swapchain->depthImage, swapchain->depthImageMemory, swapchain->depthImageView);
		swapchain->createFramebuffers(renderPass, device);
		createTextureImage();
		createTextureImageView();
		createTextureSampler();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
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
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;
	}

	void setupDebugMessenger() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);
		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != 
				VK_SUCCESS) {
			throw std::runtime_error("failed to setup debug messenger!");
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void createInstance() {

		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
		
		// Vulkan App Info
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan Template";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = Engine_NAME;
		appInfo.engineVersion = VK_MAKE_VERSION(Engine_VERSION_MAJOR, 
				Engine_VERSION_MINOR, Engine_VERSION_PATCH);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// Vulkan Instance Info
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// Add Vulkan extensions required to Instance info
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// Add validation layers to instance info
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

#if (defined(__APPLE__))
		// Added to support MoltenVK for macOS
		createInfo.flags = createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
		
		// Create instance
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
		
	}

	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		
		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}


#if (defined(__APPLE__))
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
		active_instance_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
		active_instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
		active_instance_extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
		active_instance_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
		active_instance_extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
		active_instance_extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
		active_instance_extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#endif
		return extensions;
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		std::cout << "Available layers: " << std::endl;
		for (const auto& layerProperties: availableLayers)
		{
			std::cout << layerProperties.layerName << std::endl;
		}

		for (const char* layerName : validationLayers) {
			bool layerFound = false;
			for (const auto& layerProperties: availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					std::cout << "Layer found: " << layerName << std::endl;
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				std::cout << "Layer not found: " << layerName << std::endl;
				return false;
			}
		}
	
		return true;
	}



	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, const Mesh<VertexPosColTex> &mesh) {
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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineFactory->getVkPipeline(ENG_SHADER::PosColTex));

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

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

		VkBuffer vertexBuffers[] = {vertexBuffer[0]};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, indexBuffer[0], 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);

		//assert(graphicsPipelines.size() >= 2);
		//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines.at(1));

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
		recordCommandBuffer(commands->commandBuffers[currentFrame], imageIndex, sceneState.posColTexMeshes.at(0));
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

	template <typename T>
	void createVertexBuffer(const std::vector<Mesh<T>> &meshes, VkBuffer &vertexBuffer, VkDeviceMemory &vertexBufferMemory)
	{
		const auto vert_size = sizeof(meshes[0].vertices[0]);
		VkDeviceSize bufferSize = vert_size * meshes[0].vertices.size();
		if (meshes.size() > 1)
		{
			for (auto mesh_it = meshes.begin() + 1; mesh_it != meshes.end(); ++mesh_it)
			{
				bufferSize += vert_size * mesh_it->vertices.size();
			}
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		char* last_byte = (char*) data + bufferSize;
		for (const auto& mesh : meshes)
		{
			size_t buffSize = (size_t) vert_size * mesh.vertices.size();
			assert((char*)data + buffSize <= last_byte);
			memcpy(data, mesh.vertices.data(), buffSize);
			data = (char*) data + buffSize;
		}
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
		std::cout << "Vertex buffer created successfully" << std::endl;
	}

	template <typename T>
	void createIndexBuffer(const std::vector<Mesh<T>> &meshes, VkBuffer &indexBuffer, VkDeviceMemory &indexBufferMemory)
	{
		size_t idx_size = sizeof(meshes[0].indices[0]);
		VkDeviceSize bufferSize = idx_size * meshes[0].indices.size();
		if (meshes.size() > 1)
		{
			for (auto mesh_it = meshes.begin() + 1; mesh_it != meshes.end(); ++mesh_it)
			{
				std::cout << "Iterating meshes total length" << std::endl;
				bufferSize += idx_size * mesh_it->indices.size();
			}
		}
		std::cout << "buffer size: " << bufferSize << std::endl;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		char* last_byte = (char*) data + bufferSize;
		for (const auto& mesh : meshes)
		{
			size_t buffSize = (size_t) idx_size * mesh.indices.size();
			assert((char*)data + buffSize <= last_byte);
			memcpy(data, mesh.indices.data(), buffSize);
			data = (char*) data + buffSize;
		}
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
		std::cout << "idx buffer created successfully" << std::endl;
	}


	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = Device::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = commands->beginSingleTimeCommands(device);

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		commands->endSingleTimeCommands(device, graphicsQueue, commandBuffer);
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = commands->beginSingleTimeCommands(device);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = {0, 0, 0};
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		commands->endSingleTimeCommands(device, graphicsQueue, commandBuffer);
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = commands->beginSingleTimeCommands(device);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		commands->endSingleTimeCommands(device, graphicsQueue, commandBuffer);
	}


	void createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	void updateUniformBuffer(uint32_t currentImage) {
		UniformBufferObject ubo{};
		// ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.model = sceneState.test_model;
		glm::vec3 test_pos{ 2.f, 2.f, 2.f };

		const auto& camera_node = sceneState.scene.nodes[sceneState.activeCameraNodeIdx];
		const auto& camera_position = sceneState.scene.nodes[sceneState.activeCameraNodeIdx].translation;
		const glm::vec3 glm_cam_pos = test_pos;  // { camera_position[0], camera_position[1], camera_position[2] };

		ubo.view = glm::lookAt(glm_cam_pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		//const auto& cam_rot = camera_node.rotation;
		//if (cam_rot.size() == 4)
		//{
		//	auto quat1 = glm::quat(cam_rot[3], cam_rot[0], cam_rot[1], cam_rot[2]);
		//	ubo.view = glm::mat4_cast(quat1);
			//ubo.view[0][3] = glm_cam_pos.x;
			//ubo.view[1][3] = glm_cam_pos.y;
			//ubo.view[2][3] = glm_cam_pos.z;
		//}

		const auto& camera = sceneState.scene.cameras[camera_node.camera];
		const auto fovy = static_cast<float>(camera.perspective.yfov);
		const auto aspect = static_cast<float>(camera.perspective.aspectRatio);
		const auto znear = static_cast<float>(camera.perspective.znear);
		const auto zfar = static_cast<float>(camera.perspective.zfar);
		ubo.proj = glm::perspective(fovy, aspect, znear, zfar);
		ubo.proj[1][1] *= -1;

		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}

	void createDescriptorPool() {
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void createDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView;
			imageInfo.sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

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

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);

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

		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
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
		init_info.Instance = instance;
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


	void drawGUI() {
		// Our state
		bool show_demo_window = true;
		bool show_another_window = false;
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
		{
			ImGui::ShowDemoWindow(&show_demo_window);
		}

		// Rendering
		ImGui::Render();
	}
};

int main() {
	
	try {
		printf("Starting app\n");
		// std::cout << "Application path: " << install_dir.native().c_str() << std::endl;

		VulkanTemplateApp app;
		std::cout << app;

		// std::cout << "GLTF path: " << gltf_dir.native().c_str() << std::endl;
		load_gltf(get_gltf_dir(), app.sceneState);
		
		app.loadMeshesToVkBuffer(app.sceneState);

		std::cout << "PosColTex Meshes loaded:" << std::endl;
		for (const auto& mesh : app.sceneState.posColTexMeshes)
		{
			std::cout << "\t" << mesh.name << std::endl;
		}

		std::cout << "PosNorTex Meshes loaded:" << std::endl;
		for (const auto& mesh : app.sceneState.posNorTexMeshes)
		{
			std::cout << "\t" << mesh.name << std::endl;
		}

		app.run();

	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	    	return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
