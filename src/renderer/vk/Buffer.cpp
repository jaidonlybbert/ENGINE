#include<stdexcept>
#include<iostream>
#include "vulkan/vulkan_core.h"
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"
#include "renderer/vk/Device.hpp"
#include "renderer/vk/Buffer.hpp"
#include "logger/Logging.hpp"

namespace ENG
{

Buffer::Buffer(const VkDevice &device, const VkPhysicalDevice &physicalDevice, const size_t element_size_bytes, const VkDeviceSize size, const VkBufferUsageFlags usage,
	       const VkMemoryPropertyFlags properties) : device(device), physicalDevice(physicalDevice), total_size_bytes(size), element_size_bytes(element_size_bytes)
{
	createBuffer(size, usage, properties);
}

Buffer::~Buffer()
{
	ENG_LOG_TRACE("Buffer destruction! at address " << &buffer << std::endl);
	vkDestroyBuffer(device, buffer, nullptr);
	vkFreeMemory(device, bufferMemory, nullptr);
}

void Buffer::createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties)
{
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

	if (vkBindBufferMemory(device, buffer, bufferMemory, 0) != VK_SUCCESS) {
		throw std::runtime_error("failed to bind memory!");
	}
}
} // end namespace
