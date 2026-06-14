#ifndef VK_ADAPTER_HPP
#define VK_ADAPTER_HPP
#include "scene/Scene.hpp"
#include "renderer/vk/Renderer.hpp"
#include "renderer/RendererI.hpp"
#include "application/ConcurrentQueue.hpp"


enum DrawDataProperties : uint32_t {
	CLEAR = 0x0,
	DESCRIPTOR_SETS_INITIALIZED = 0x1,
	VERTEX_BUFFERS_INITIALIZED = 0x2,
	INDEX_BUFFERS_INITIALIZED = 0x4,
	INDEXED_DRAW = 0x8,
};

struct DrawDataAllocationInfo {
	VkBuffer vertexBuffers[1];
	VkDeviceSize vertexBufferOffsets[1]{ 0 };
	VmaAllocation vertexAllocation;
	VmaAllocationInfo vertexAllocationInfo;
	VkBuffer indexBuffer;
	VmaAllocation indexAllocation;
	VmaAllocationInfo indexAllocationInfo;
	uint32_t indexCount;
    uint32_t vertexCount;
};

struct alignas(CACHE_LINE_SIZE) DrawData
{
	uint32_t propertyFlags{ DrawDataProperties::CLEAR };
	std::optional<uint32_t> nodeId;
	std::optional<std::filesystem::path> texturePath;
	std::optional<std::vector<VkDescriptorSet>> descriptorSets;
	std::optional<DrawDataAllocationInfo> bufferAllocationInfo;
};


struct CommandRecorderEvent {
	std::function<void(VkCommandBuffer)> commandRecorder;
};

struct CommandCompletionEvent {
	std::function<void(void)> commandCompletionHandler;
};

using GraphicsEvent = std::variant<
	BindHostMeshDataEvent,
	CommandRecorderEvent,
	CommandCompletionEvent
>;

class VkAdapter : RenderAdapterI
{
private:
	mutable std::mutex drawDataMutex;

public:
	VkCommandPool cmdPool{};
	VkRenderer& renderer;
	VmaAllocator vmaAllocator;
	std::vector<DrawData> drawDataBuffer;

	ConcurrentQueue<GraphicsEvent> graphicsEventQueue{};

	VkAdapter(VkRenderer& renderer) : renderer(renderer)
	{
		drawDataBuffer.reserve(10000);
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.vulkanApiVersion = Engine_VK_API_VERSION;
		allocatorCreateInfo.physicalDevice = renderer.physicalDevice;
		allocatorCreateInfo.device = renderer.device;
		allocatorCreateInfo.instance = renderer.instanceFactory->instance;
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);


	}

	bool has_property(const size_t drawDataIdx, const DrawDataProperties propertyEnum)
	{
		std::lock_guard lock(drawDataMutex);
		assert(drawDataIdx < drawDataBuffer.size());
		const auto& drawData = drawDataBuffer.at(drawDataIdx);
		return (drawData.propertyFlags & propertyEnum) != 0;
	}

	void set_property(const size_t drawDataIdx, const DrawDataProperties propertyEnum)
	{
		std::lock_guard lock(drawDataMutex);
		assert(drawDataIdx < drawDataBuffer.size());
		auto& drawData = drawDataBuffer.at(drawDataIdx);
		drawData.propertyFlags |= propertyEnum;
	}

	~VkAdapter()
	{
		for (auto& drawData : drawDataBuffer)
		{
			if (drawData.bufferAllocationInfo.has_value())
			{
				vmaDestroyBuffer(
					vmaAllocator,
					drawData.bufferAllocationInfo.value().vertexBuffers[0],
					drawData.bufferAllocationInfo.value().vertexAllocation);
				vmaDestroyBuffer(
					vmaAllocator,
					drawData.bufferAllocationInfo.value().indexBuffer,
					drawData.bufferAllocationInfo.value().indexAllocation);
			}
		}
		vmaDestroyAllocator(vmaAllocator);
	}

	void copyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
		VkBufferCopy region{};
		region.size = size;
		vkCmdCopyBuffer(cmd, src, dst, 1, &region);
	}

	DrawDataAllocationInfo create_draw_data(
		VertexT&& vertices, 
		std::vector<uint32_t>&& indices)
	{
		DrawDataAllocationInfo drawDataInfo{};
		drawDataInfo.indexCount = static_cast<uint32_t>(indices.size());
        
        
        auto vertexSize{0};
        if (std::holds_alternative<std::vector<VertexPosColTex>>(vertices)) {
            drawDataInfo.vertexCount = static_cast<uint32_t>(std::get<std::vector<VertexPosColTex>>(vertices).size());
            vertexSize = drawDataInfo.vertexCount * sizeof(VertexPosColTex);
        } else if (std::holds_alternative<std::vector<VertexPosNorCol>>(vertices)) {
            drawDataInfo.vertexCount = static_cast<uint32_t>(std::get<std::vector<VertexPosNorCol>>(vertices).size());
            vertexSize = drawDataInfo.vertexCount * sizeof(VertexPosNorCol);
        } else if (std::holds_alternative<std::vector<VertexPosNorTex>>(vertices)) {
            drawDataInfo.vertexCount = static_cast<uint32_t>(std::get<std::vector<VertexPosNorTex>>(vertices).size());
            vertexSize = drawDataInfo.vertexCount * sizeof(VertexPosNorTex);
        } else {
            drawDataInfo.vertexCount = 0;
            vertexSize = 0;
        }


		const auto* vertexData =
			std::holds_alternative<std::vector<VertexPosColTex>>(vertices) ? static_cast<void*>(std::get<std::vector<VertexPosColTex>>(vertices).data()) :
			std::holds_alternative<std::vector<VertexPosNorCol>>(vertices) ? static_cast<void*>(std::get<std::vector<VertexPosNorCol>>(vertices).data()) :
			std::holds_alternative<std::vector<VertexPosNorTex>>(vertices) ? static_cast<void*>(std::get<std::vector<VertexPosNorTex>>(vertices).data()) :
			nullptr;

		if (!vertexData)
		{
			ENG_LOG_ERROR("Vertex data is null!" << std::endl);
			return {};
		}

		VkDeviceSize indexSize = sizeof(uint32_t) * indices.size(); 

		ENG_LOG_INFO("Binding vertex and index buffers of size: (" << vertexSize << "," << indexSize << ")" << std::endl);
		
		VkBufferCreateInfo vbInfo{}; 
		vbInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; 
		vbInfo.size = vertexSize; 
		vbInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; 

		VkBufferCreateInfo ibInfo{}; 
		ibInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; 
		ibInfo.size = indexSize; 
		ibInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; 

		VmaAllocationCreateInfo gpuAlloc{}; 
		gpuAlloc.usage = VMA_MEMORY_USAGE_GPU_ONLY; 

		vmaCreateBuffer(vmaAllocator, &vbInfo, &gpuAlloc, &drawDataInfo.vertexBuffers[0], &drawDataInfo.vertexAllocation, &drawDataInfo.vertexAllocationInfo);
		vmaCreateBuffer(vmaAllocator, &ibInfo, &gpuAlloc, &drawDataInfo.indexBuffer, &drawDataInfo.indexAllocation, &drawDataInfo.indexAllocationInfo);

		// copy vertex data to device staging buffer
		// Staging buffer creation info
		VmaAllocationCreateInfo stagingAlloc{};
		stagingAlloc.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		VkBuffer stagingVB;
		VmaAllocation stagingVBAlloc;

		VkBufferCreateInfo stagingInfoVB = vbInfo;
		stagingInfoVB.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		vmaCreateBuffer(vmaAllocator, &stagingInfoVB, &stagingAlloc,
			&stagingVB, &stagingVBAlloc, nullptr);

		// Map + copy vertex data
		void* data;
		vmaMapMemory(vmaAllocator, stagingVBAlloc, &data);
		memcpy(data, vertexData, vertexSize);
		vmaUnmapMemory(vmaAllocator, stagingVBAlloc);

		// copy index data over to staging buffer
		VkBuffer stagingIB;
		VmaAllocation stagingIBAlloc;

		VkBufferCreateInfo stagingInfoIB = ibInfo;
		stagingInfoIB.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		vmaCreateBuffer(vmaAllocator, &stagingInfoIB, &stagingAlloc,
						&stagingIB, &stagingIBAlloc, nullptr);

		vmaMapMemory(vmaAllocator, stagingIBAlloc, &data);
		memcpy(data, indices.data(), indexSize);
		vmaUnmapMemory(vmaAllocator, stagingIBAlloc);

		graphicsEventQueue.push(
			CommandRecorderEvent{
				[this, stagingVB, stagingIB, stagingVBAlloc, stagingIBAlloc, drawDataInfo, vertexSize, indexSize](VkCommandBuffer cmdBuffer)
				{
					copyBuffer(cmdBuffer, stagingVB, drawDataInfo.vertexBuffers[0], vertexSize);
					copyBuffer(cmdBuffer, stagingIB, drawDataInfo.indexBuffer, indexSize);
				}
			}
		);

		graphicsEventQueue.push(
			CommandCompletionEvent{
				[this, stagingVB, stagingIB, stagingVBAlloc, stagingIBAlloc]()
				{
					vmaDestroyBuffer(vmaAllocator, stagingVB, stagingVBAlloc);
					vmaDestroyBuffer(vmaAllocator, stagingIB, stagingIBAlloc);
				}
			}
		);

		return drawDataInfo;
	}

	void createDescriptorSets(const size_t drawDataIdx, ENG::Node& node)
	{
		std::lock_guard lock(drawDataMutex);

		auto& drawData{ drawDataBuffer.at(drawDataIdx) };

		if (!drawData.bufferAllocationInfo.has_value())
		{
			ENG_LOG_ERROR("Attempted to create descriptor sets for draw data with no DrawDataBufferAllocationInfo" << std::endl);
			return;
		}

		if (!drawData.nodeId.has_value())
		{
			ENG_LOG_ERROR("Attempted to create descriptor set for DrawData with no nodeId" << std::endl);
			return;
		}

		const auto& bufferAllocationInfo = drawData.bufferAllocationInfo.value();

		if (!node.shaderId.has_value())
		{
			ENG_LOG_ERROR("Attempted to create descriptor set for DrawData with no shaderId" << std::endl);
			return;
		}

		ENG_LOG_INFO("Writing descriptor sets for: " << node.name << std::endl);

		drawData.descriptorSets = std::vector<VkDescriptorSet>{};
		assert(drawData.descriptorSets.has_value());
		renderer.createDescriptorSets(drawData.descriptorSets.value(), node.shaderId.value(), drawData.texturePath);
	}

	/*
	* Record commands and submit to queue, must be called from main thread
	*/
	void command_recorder_event_handler(CommandRecorderEvent&& commandRecorderEvent) {
		VkCommandBuffer cmdBuffer = renderer.commands->beginSingleTimeCommands(renderer.device);

		commandRecorderEvent.commandRecorder(cmdBuffer);

		renderer.commands->endSingleTimeCommands(renderer.device, renderer.graphicsQueue, cmdBuffer);
	}


	/*
	* Returns index of allocated draw data 
	*/
	size_t emplaceDrawData(DrawData&& drawData) {
		std::lock_guard<std::mutex> lock(drawDataMutex);
		drawDataBuffer.emplace_back(drawData);
		return drawDataBuffer.size() - 1;
	}

	void recordCommandsForSceneGraph2(VkRenderer& renderer, VkCommandBuffer& commandBuffer, SceneState& sceneState);

	/*
	* Returns copy of draw data at given index.
	*/
	DrawData getDrawDataFromIdx(uint32_t idx) {
		if (idx >= drawDataBuffer.size())
		{
			ENG_LOG_ERROR(
				"Attempted to access drawData at idx: " << idx 
				<< " but the buffer is only of length: " 
				<< drawDataBuffer.size() << std::endl);
			return {};
		}
		return drawDataBuffer.at(idx);
	}
};

void recordDrawCommand(VkCommandBuffer& commandBuffer, const ENG::Mesh& mesh);
#endif
