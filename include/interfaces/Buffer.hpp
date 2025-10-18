#ifndef ENG_BUFFER
#define ENG_BUFFER
#include "vulkan/vulkan_core.h"

namespace ENG
{
class Buffer
{
public:
	VkDeviceMemory bufferMemory;
	VkBuffer buffer;
	VkDeviceSize total_size_bytes;
	size_t element_size_bytes;
	const VkDevice& device;
	const VkPhysicalDevice& physicalDevice;

	explicit Buffer(const VkDevice &device, const VkPhysicalDevice &physicalDevice, const size_t element_size_bytes,
		const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties);
	~Buffer();
	void createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties);
};
}
#endif
