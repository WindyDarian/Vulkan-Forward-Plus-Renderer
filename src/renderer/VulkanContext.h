// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include "VDeleter.h"
#include "../util.h"

// TODO: DELETE ME
#include "../Scene.h"

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <array>

struct UniformBufferObject
{
	// todo merge these 3
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 cam_pos;
};



class VulkanContext
{
public:
	VulkanContext(GLFWwindow* window);

	void resize(int width, int height);
	void requestDraw(float deltatime);
	void cleanUp();

	void setCamera(const glm::mat4 & view, const glm::vec3 campos);

	static void DestroyDebugReportCallbackEXT(VkInstance instance
		, VkDebugReportCallbackEXT callback
		, const VkAllocationCallbacks* pAllocator);

	static VkResult CreateDebugReportCallbackEXT(VkInstance instance
		, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo
		, const VkAllocationCallbacks* pAllocator
		, VkDebugReportCallbackEXT* pCallback);

	

private:
	GLFWwindow* window;

	VDeleter<VkInstance> instance{ vkDestroyInstance };
	VDeleter<VkDebugReportCallbackEXT> callback{ instance, DestroyDebugReportCallbackEXT };
	VkPhysicalDevice physical_device;

	VDeleter<VkDevice> graphics_device{ vkDestroyDevice }; //logical device
	VkQueue graphics_queue;

	VDeleter<VkSurfaceKHR> window_surface{ instance, vkDestroySurfaceKHR };
	VkQueue present_queue;

	VkQueue compute_queue;

	VDeleter<VkSwapchainKHR> swap_chain{ graphics_device, vkDestroySwapchainKHR };
	std::vector<VkImage> swap_chain_images;
	VkFormat swap_chain_image_format;
	VkExtent2D swap_chain_extent;
	std::vector<VDeleter<VkImageView>> swap_chain_imageviews;
	std::vector<VDeleter<VkFramebuffer>> swap_chain_framebuffers;
	VDeleter<VkRenderPass> render_pass{ graphics_device, vkDestroyRenderPass };

	VDeleter<VkDescriptorSetLayout> descriptor_set_layout{ graphics_device, vkDestroyDescriptorSetLayout };
	VDeleter<VkPipelineLayout> pipeline_layout{ graphics_device, vkDestroyPipelineLayout };
	VDeleter<VkPipeline> graphics_pipeline{ graphics_device, vkDestroyPipeline };

	// Command buffers
	VDeleter<VkCommandPool> command_pool{ graphics_device, vkDestroyCommandPool };
	std::vector<VkCommandBuffer> command_buffers; // buffers will be released when pool destroyed

	VDeleter<VkSemaphore> image_available_semaphore{ graphics_device, vkDestroySemaphore };
	VDeleter<VkSemaphore> render_finished_semaphore{ graphics_device, vkDestroySemaphore };

	// only one image buffer for depth because only one draw operation happens at one time
	VDeleter<VkImage> depth_image{ graphics_device, vkDestroyImage };
	VDeleter<VkDeviceMemory> depth_image_memory{ graphics_device, vkFreeMemory };
	VDeleter<VkImageView> depth_image_view{ graphics_device, vkDestroyImageView };

	// texture image
	VDeleter<VkImage> texture_image{ graphics_device, vkDestroyImage };
	VDeleter<VkDeviceMemory> texture_image_memory{ graphics_device, vkFreeMemory };
	VDeleter<VkImageView> texture_image_view{ graphics_device, vkDestroyImageView };
	VDeleter<VkImage> normalmap_image{ graphics_device, vkDestroyImage };
	VDeleter<VkDeviceMemory> normalmap_image_memory{ graphics_device, vkFreeMemory };
	VDeleter<VkImageView> normalmap_image_view{ graphics_device, vkDestroyImageView };
	VDeleter<VkSampler> texture_sampler{ graphics_device, vkDestroySampler };

	// uniform buffer and descriptor
	VDeleter<VkBuffer> uniform_staging_buffer{ graphics_device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniform_staging_buffer_memory{ graphics_device, vkFreeMemory };
	VDeleter<VkBuffer> uniform_buffer{ graphics_device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniform_buffer_memory{ graphics_device, vkFreeMemory };

	VDeleter<VkDescriptorPool> descriptor_pool{ graphics_device, vkDestroyDescriptorPool };
	VkDescriptorSet descriptor_set;

	// vertex buffer
	VDeleter<VkBuffer> vertex_buffer{ this->graphics_device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> vertex_buffer_memory{ graphics_device, vkFreeMemory };
	VDeleter<VkBuffer> index_buffer{ graphics_device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> index_buffer_memory{ graphics_device, vkFreeMemory };
	
	VDeleter<VkBuffer> pointlight_buffer{ this->graphics_device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> pointlight_buffer_memory{ graphics_device, vkFreeMemory };

	std::vector<util::Vertex> vertices;
	std::vector<uint32_t> vertex_indices;

	std::vector<PointLight> pointlights;
	const int MAX_POINT_LIGHT_COUNT = 1000;
	const glm::vec3 LIGHTPOS_MIN = { -15, -10, -20 };
	const glm::vec3 LIGHTPOS_MAX = { 15, 20, 20 };

	int window_framebuffer_width;
	int window_framebuffer_height;

	glm::mat4 view_matrix;
	glm::vec3 cam_pos;

#ifdef NDEBUG
	// if not debugging
	const bool ENABLE_VALIDATION_LAYERS = false;
#else
	const bool ENABLE_VALIDATION_LAYERS = true;
#endif

	const std::vector<const char*> VALIDATION_LAYERS = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	const std::vector<const char*> DEVICE_EXTENSIONS = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	void initVulkan();

	void recreateSwapChain();

	void createInstance();
	void setupDebugCallback();
	void createWindowSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createSwapChainImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createCommandPool();
	void createDepthResources();
	void createFrameBuffers();
	void createTextureAndNormal();
	void createTextureSampler();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffer();
	void createLights();
	void createDescriptorPool();
	void createDescriptorSet();
	void createCommandBuffers();
	void createSemaphores();
	void createComputePipeline();

	void updateUniformBuffer(float deltatime);
	void drawFrame();

	void createShaderModule(const std::vector<char>& code, VkShaderModule* p_shader_module);

	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	inline VkFormat findDepthFormat()
	{
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }
			, VK_IMAGE_TILING_OPTIMAL
			, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags property_bits
		, VkBuffer* p_buffer, VkDeviceMemory* p_buffer_memory);
	void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

	void createImage(uint32_t image_width, uint32_t image_height
		, VkFormat format, VkImageTiling tiling
		, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties
		, VkImage* p_vkimage, VkDeviceMemory* p_image_memory);
	void copyImage(VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height);
	void transitImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);

	void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_mask, VkImageView * p_image_view);

	void loadImageFromFile(std::string path, VkImage* p_vkimage, VkDeviceMemory* p_image_memory, VkImageView * p_image_view);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	// Called on vulcan command buffer recording
	void recordCopyBuffer(VkCommandBuffer command_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
	void recordCopyImage(VkCommandBuffer command_buffer, VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height);
	void recordTransitImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);
};
