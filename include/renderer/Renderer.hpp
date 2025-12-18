#pragma once
#include<iterator>
#include<stack>
#include<vector>
#include<memory>

#include "vulkan/vulkan_core.h"

#include "lua.hpp"

#include "scene/Scene.hpp"
#include "renderer/Instance.hpp"
#include "renderer/Swapchain.hpp"
#include "renderer/pipelines/PipelineFactory.hpp"

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


class VulkanTemplateApp {
public:
	VulkanTemplateApp();
	void run();
	void initialize();
	void initializeScene(std::function<void(VulkanTemplateApp&)> loadFunction);
	~VulkanTemplateApp();
	void cleanupGui();
	friend std::ostream& operator<<(std::ostream& os, VulkanTemplateApp& app);

	SceneState sceneState;
	GameState gameState;
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
	std::vector<std::function<void(VkCommandBuffer)>> commandRecorders;
	std::vector<std::function<void(void)>> initializationFunctions;
	std::vector<std::function<void(void)>> renderStateUpdaters;

	std::mutex scene_mtx;
	bool sceneReadyToRender = false;

	void registerRenderStateUpdater(std::function<void(void)> renderStateUpdater);
	void registerInitializationFunction(std::function<void(void)> initFunc);
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	void initVulkan();
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void createSurface();
	void recordCommandsForSceneGraph(VkCommandBuffer& commandBuffer);
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void registerCommandRecorder(std::function<void(VkCommandBuffer)> commandRecorder);
	void drawFrame();
	void createSyncObjects();
	void createUniformBuffers();

	/// <summary>
	/// Must be called after all nodes are loaded
	/// </summary>
	void createModelMatrices();
	void createFaceIdBuffers(const uint32_t number_of_faces);
	void createFaceColorBuffers(const uint32_t number_of_faces);
	void updateModelMatrix(glm::mat4& modelMatrix, const ENG::Node& node);


	void updateModelMatrixBuffer();
	void updateUniformBuffer(uint32_t currentImage);
	void createDescriptorPool();

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

	void writeDescriptorSets(const ENG::Node& node);
	void createDescriptorSets(ENG::Node& node);
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void initGui();
};
