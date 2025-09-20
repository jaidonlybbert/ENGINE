#include "interfaces/PhysicalDevice.h"
#include "vulkan/vulkan_core.h"
#include<vector>
#include<iostream>
#include<set>

namespace ENG
{

bool QueueFamilyIndices::isComplete() {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, 
			nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, 
			availableExtensions.data());

	std::cout << "Available Ext" << std::endl;
	for (auto ext : availableExtensions) {
		std::cout << ext.extensionName << std::endl;
	}

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	std::cout << "Required Ext" << std::endl;
	for (auto ext : requiredExtensions) {
		std::cout << ext << std::endl;
	}

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices PhysicalDevice::findQueueFamilies(VkPhysicalDevice device, const VkSurfaceKHR &surface) {
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, 
			&queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
			queueFamilies.data());
	// report queue family that supports GRAPHICS_BIT queue
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, 
				&presentSupport);
		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}
		i++;
	}
	return indices;
}

SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, 
				&presentModeCount, details.presentModes.data());
	}
	return details;
}

bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device, const VkSurfaceKHR &surface) {
	VkPhysicalDeviceProperties deviceProperties;
	// std::cout << deviceProperties.sType << std::endl;
	// std::cout << VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 << std::endl;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// check device supports required queue families
	QueueFamilyIndices indices = findQueueFamilies(device, surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		std::cout << "Extensions supported" << std::endl;
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && 
			!swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

void PhysicalDevice::pickPhysicalDevice(const VkInstance& instance, VkPhysicalDevice &physicalDevice, const VkSurfaceKHR &surface) {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	std::cout << "Device count: " << deviceCount << std::endl;

	for (const auto& device : devices) {
		if (isDeviceSuitable(device, surface)) {
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

} // end namespace
