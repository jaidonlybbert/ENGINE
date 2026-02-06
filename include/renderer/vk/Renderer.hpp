#pragma once
#include<iterator>
#include<stack>
#include<vector>
#include<memory>
#include<functional>

#include "vulkan/vulkan_core.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"

#include "renderer/vk/pipelines/Pipeline.hpp"
#include "renderer/vk/pipelines/PipelineFactory.hpp"
#include "renderer/vk/Instance.hpp"
#include "renderer/vk/Buffer.hpp"
#include "renderer/vk/Command.hpp"
#include "renderer/vk/Swapchain.hpp"
#include "scene/Scene.hpp"


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


class VkRenderer {
public:
	VkRenderer(bool& framebufferResized, std::vector<std::function<void(void)>> initFunctions,
		std::vector<std::function<void(void)>> cleanupFunctions);
	void initialize();
	~VkRenderer();
	void cleanupGui();
	void cleanupVulkan();
	void cleanupWindow();
	friend std::ostream& operator<<(std::ostream& os, VkRenderer& app);

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
	uint32_t currentFrame = 0;
	bool& framebufferResized;
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

	std::unordered_map<std::filesystem::path, VkImage> textureImages;
	std::unordered_map<std::filesystem::path, VkDeviceMemory> textureImageMemory;
	std::unordered_map<std::filesystem::path, VkImageView> textureImageViews;
	std::unordered_map<std::filesystem::path, VkSampler> textureSamplers;

	std::unique_ptr<ENG::InstanceFactory> instanceFactory;
	std::unique_ptr<ENG::PipelineFactory> pipelineFactory;
	std::unique_ptr<ENG::Command> commands;
	std::unique_ptr<ENG::Swapchain> swapchain;
	std::vector<std::function<void(VkCommandBuffer)>> commandRecorders;
	std::vector<std::function<void(void)>> initializationFunctions;
	std::vector<std::function<void(void)>> cleanupFunctions;
	std::vector<std::function<void(void)>> renderStateUpdaters;
	std::function<UniformBufferObject(void)> uniformBufferUpdateFunction;
	std::function<std::vector<glm::mat4>&(void)> modelMatrixBufferUpdateFunction;

	std::mutex scene_mtx;
	bool sceneReadyToRender = false;

	void registerInitializationFunction(std::function<void(void)> initFunc);
	void initVulkan();
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void createSurface();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void registerCommandRecorder(std::function<void(VkCommandBuffer)> commandRecorder);
	void drawFrame();
	void createSyncObjects();
	void createRenderFinishedSemaphores();
	void destroyRenderFinishedSemaphores();
	void recreateRenderFinishedSemaphores();
	void createUniformBuffers();

	/// <summary>
	/// Must be called after all nodes are loaded
	/// </summary>
	void createModelMatrices(const size_t size_bytes);
	void createFaceIdBuffers(const uint32_t number_of_faces);
	void createFaceColorBuffers(const uint32_t number_of_faces);

	void registerUniformBufferProducer(std::function<UniformBufferObject(void)> producer);
	void registerModelMatrixBufferUpdateFunction(std::function<std::vector<glm::mat4>&(void)> bufferUpdater);

	void copyModelMatrixBufferToGpu(const std::vector<glm::mat4>& modelMatrices);
	void copyUniformBufferToGpu(const uint32_t currentImage, const UniformBufferObject& ubo);

	void createDescriptorPool();

	VkWriteDescriptorSet createWriteDescriptorSet(
		const VkDescriptorSet descriptorSet,
		const VkDescriptorBufferInfo bufferInfo,
		const VkDescriptorType descriptorType,
		const size_t bindingIdx
	);

	VkWriteDescriptorSet createWriteDescriptorSet(
		const VkDescriptorSet descriptorSet,
		const VkDescriptorImageInfo imageInfo,
		const VkDescriptorType descriptorType,
		const size_t bindingIdx
	);

	VkWriteDescriptorSet createDescriptorWriteModelMatrix(
		const ENG::Node& node, 
		const size_t frameIdx, 
		const size_t bindingIdx, 
		const VkDescriptorBufferInfo& modelMatrixBufferInfo);

	VkWriteDescriptorSet createDescriptorWriteSampler(
		const ENG::Node& node, 
		const size_t frameIdx, 
		const size_t bindingIdx, 
		const VkDescriptorImageInfo& imageInfo);

	VkWriteDescriptorSet createDescriptorWriteUbo(
		const ENG::Node& node, 
		const size_t frameIdx, 
		const size_t bindingIdx, 
		const VkDescriptorBufferInfo& bufferInfo);

	VkWriteDescriptorSet createDescriptorWriteFaceColorMatrix(
		const ENG::Node& node,
		const size_t frameIdx,
		const size_t bindingIdx,
		const VkDescriptorBufferInfo& bufferInfo);

	VkWriteDescriptorSet createDescriptorWriteFaceIdMapBuffer(
		const ENG::Node& node,
		const size_t frameIdx,
		const size_t bindingIdx,
		const VkDescriptorBufferInfo& bufferInfo);

	void writeDescriptorSets(
		const std::vector<VkDescriptorSet>& descriptorSets, 
		const std::string& shaderId,
		const std::optional<std::filesystem::path> texturePath);

	void createDescriptorSets(ENG::Node& node);

	void createDescriptorSets(
		std::vector<VkDescriptorSet>& descriptorSetsP, const std::string& shaderId, const std::optional<std::filesystem::path> texturePath);

	void createTextureImage(const std::filesystem::path& fpath);
	void createTextureImageView(const std::filesystem::path& fpath);
	void createTextureSampler(const std::filesystem::path& fpath);
	void createTexture(const std::filesystem::path& fpath);
	void initGui();
};
