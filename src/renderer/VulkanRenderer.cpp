// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

// We are mixing up Vulkan C binding (vulkan.h) and C++ binding 
//  (vulkan.hpp), because we are trying to use C++ binding for
// new codes; cleaning up will be done at some point

#include "VulkanRenderer.h"

#include "raii.h"
#include "../util.h"
#include "vulkan_util.h"
#include "context.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <stb_image.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <functional>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <fstream>
#include <chrono>

using util::Vertex;

const int MAX_POINT_LIGHT_COUNT = 1000;
const int MAX_POINT_LIGHT_PER_TILE = 63;
const int TILE_SIZE = 16;

struct PointLight
{
public:
	//glm::vec3 pos = { 0.0f, 1.0f, 0.0f };
	glm::vec3 pos;
	float radius = { 5.0f };
	glm::vec3 intensity = { 1.0f, 1.0f, 1.0f };
	float padding;

	PointLight() {}
	PointLight(glm::vec3 pos, float radius, glm::vec3 intensity)
		: pos(pos), radius(radius), intensity(intensity)
	{};
};


// uniform buffer object for model transformation
struct SceneObjectUbo
{
	glm::mat4 model;
};

// uniform buffer object for camera
struct CameraUbo
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 projview;
	glm::vec3 cam_pos;
};

struct PushConstantObject
{
	glm::ivec2 viewport_size;
	glm::ivec2 tile_nums;
	int debugview_index; // TODO: separate this and only have it in debug mode?

	PushConstantObject(int viewport_size_x, int viewport_size_y, int tile_num_x, int tile_num_y, int debugview_index = 0)
		: viewport_size(viewport_size_x, viewport_size_y),
		tile_nums(tile_num_x, tile_num_y),
		debugview_index(debugview_index)
	{}
};



class _VulkanRenderer_Impl
{
public:
	_VulkanRenderer_Impl(GLFWwindow* window);

	void resize(int width, int height);
	void requestDraw(float deltatime);
	void cleanUp();

	void setCamera(const glm::mat4 & view, const glm::vec3 campos);


	int getDebugViewIndex() const
	{
		return debug_view_index;
	}

	/**
	*  0: render 1: heat map with render 2: heat map 3: depth 4: normal
	*/
	void changeDebugViewIndex(int target_view)
	{
		debug_view_index = target_view % 5;
		recreateSwapChain(); // TODO: change this to a state modification and handle the recreation before update
	}

private:

	VContext vulkan_context;

	/////////////////////////////////////////////////////
	// TODO: remove those
	QueueFamilyIndices queue_family_indices;
	VkPhysicalDevice physical_device;
	//VDeleter<VkDevice> graphics_device{ vkDestroyDevice }; //logical device
	VkDevice graphics_device;
	vk::Device device; // vulkan.hpp wraper for graphics_device, maybe I should migrate all code to vulkan-hpp
	vk::Queue graphics_queue;
	vk::Queue present_queue;
	vk::Queue compute_queue;
	//////////////////////////////////////////////////////
	
	VRaii<vk::SwapchainKHR> swap_chain;
	std::vector<VkImage> swap_chain_images;
	VkFormat swap_chain_image_format;
	VkExtent2D swap_chain_extent;
	std::vector<VRaii<vk::ImageView>> swap_chain_imageviews;
	std::vector<VRaii<vk::Framebuffer>> swap_chain_framebuffers;
	VRaii<vk::Framebuffer> depth_pre_pass_framebuffer;

	VRaii<vk::RenderPass> render_pass;
	VRaii<vk::RenderPass> depth_pre_pass; // the depth prepass which happens before formal render pass

	VRaii<vk::DescriptorSetLayout> object_descriptor_set_layout;
	VRaii<vk::DescriptorSetLayout> camera_descriptor_set_layout;
	VRaii<VkPipelineLayout> pipeline_layout;
	VRaii<VkPipeline> graphics_pipeline;
	VRaii<vk::PipelineLayout> depth_pipeline_layout;
	VRaii<vk::Pipeline> depth_pipeline;

	VRaii<vk::DescriptorSetLayout> light_culling_descriptor_set_layout;  // shared between compute queue and graphics queue
	VRaii<vk::DescriptorSetLayout> intermediate_descriptor_set_layout; // which is exclusive to compute queue
	VRaii<VkPipelineLayout> compute_pipeline_layout;
	VRaii<VkPipeline> compute_pipeline;
	VRaii<vk::CommandPool> compute_command_pool;
	vk::CommandBuffer light_culling_command_buffer = VK_NULL_HANDLE;
	//VRaii<vk::PipelineLayout> compute_pipeline_layout;
	//VRaii<vk::Pipeline> compute_pipeline;

	// Command buffers
	VRaii<vk::CommandPool> command_pool;
	std::vector<VkCommandBuffer> command_buffers; // buffers will be released when pool destroyed
	vk::CommandBuffer depth_prepass_command_buffer;

	VRaii<vk::Semaphore> image_available_semaphore;
	VRaii<vk::Semaphore> render_finished_semaphore;
	VRaii<vk::Semaphore> lightculling_completed_semaphore;
	VRaii<vk::Semaphore> depth_prepass_finished_semaphore;

	// only one image buffer for depth because only one draw operation happens at one time
	VRaii<VkImage> depth_image;
	VRaii<VkDeviceMemory> depth_image_memory;
	VRaii<VkImageView> depth_image_view;
	// for depth pre pass
	VRaii<VkImage> pre_pass_depth_image;
	VRaii<VkDeviceMemory> pre_pass_depth_image_memory;
	VRaii<VkImageView> pre_pass_depth_image_view;

	// texture image
	VRaii<VkImage> texture_image;
	VRaii<VkDeviceMemory> texture_image_memory;
	VRaii<VkImageView> texture_image_view;
	VRaii<VkImage> normalmap_image;
	VRaii<VkDeviceMemory> normalmap_image_memory;
	VRaii<VkImageView> normalmap_image_view;
	VRaii<VkSampler> texture_sampler;

	// uniform buffers
	VRaii<VkBuffer> object_staging_buffer;
	VRaii<VkDeviceMemory> object_staging_buffer_memory;
	VRaii<VkBuffer> object_uniform_buffer;
	VRaii<VkDeviceMemory> object_uniform_buffer_memory;
	VRaii<VkBuffer> camera_staging_buffer;
	VRaii<VkDeviceMemory> camera_staging_buffer_memory;
	VRaii<VkBuffer> camera_uniform_buffer;
	VRaii<VkDeviceMemory> camera_uniform_buffer_memory;

	VRaii<VkDescriptorPool> descriptor_pool;
	VkDescriptorSet object_descriptor_set;
	vk::DescriptorSet camera_descriptor_set;
	VkDescriptorSet light_culling_descriptor_set;
	vk::DescriptorSet intermediate_descriptor_set;

	// vertex buffer
	VRaii<VkBuffer> vertex_buffer;
	VRaii<VkDeviceMemory> vertex_buffer_memory;
	VRaii<VkBuffer> index_buffer;
	VRaii<VkDeviceMemory> index_buffer_memory;

	VRaii<VkBuffer> pointlight_buffer;
	VRaii<VkDeviceMemory> pointlight_buffer_memory;
	VRaii<VkBuffer> lights_staging_buffer;
	VRaii<VkDeviceMemory> lights_staging_buffer_memory;
	VkDeviceSize pointlight_buffer_size;

	std::vector<util::Vertex> vertices;
	std::vector<uint32_t> vertex_indices;

	std::vector<PointLight> pointlights;
	const glm::vec3 LIGHTPOS_MIN = { -15, -5, -5 };
	const glm::vec3 LIGHTPOS_MAX = { 15, 20, 5 };

	// This storage buffer stores visible lights for each tile
	// which is output from the light culling compute shader
	// max MAX_POINT_LIGHT_PER_TILE point lights per tile
	VRaii<VkBuffer> light_visibility_buffer;
	VRaii<VkDeviceMemory> light_visibility_buffer_memory;
	VkDeviceSize light_visibility_buffer_size = 0;

	int window_framebuffer_width;
	int window_framebuffer_height;

	glm::mat4 view_matrix;
	glm::vec3 cam_pos;
	int tile_count_per_row;
	int tile_count_per_col;
	int debug_view_index = 0; 

	void initialize() 
	{
		createSwapChain();
		createSwapChainImageViews();
		createRenderPasses();
		createDescriptorSetLayouts();
		createGraphicsPipelines();
		createComputePipeline();
		createCommandPool();
		createDepthResources();
		createFrameBuffers();
		createTextureAndNormal();
		createTextureSampler();
		std::tie(vertices, vertex_indices) = util::loadModel();
		// TODO: better to use a single memory allocation for multiple buffers
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createLights();
		createDescriptorPool();
		createSceneObjectDescriptorSet();
		createCameraDescriptorSet();
		createIntermediateDescriptorSet();
		updateIntermediateDescriptorSet();
		createLigutCullingDescriptorSet();
		createLightVisibilityBuffer(); // create a light visiblity buffer and update descriptor sets, need to rerun after changing size
		createGraphicsCommandBuffers();
		createLightCullingCommandBuffer();
		createDepthPrePassCommandBuffer();
		createSemaphores();
	}

	void recreateSwapChain() 
	{
		vkDeviceWaitIdle(graphics_device);

		createSwapChain();
		createSwapChainImageViews();
		createRenderPasses();
		createGraphicsPipelines();
		createDepthResources();
		createFrameBuffers();
		createLightVisibilityBuffer(); // since it's size will scale with window;
		updateIntermediateDescriptorSet();
		createGraphicsCommandBuffers();
		createLightCullingCommandBuffer(); // it needs light_visibility_buffer_size, which is changed on resize
		createDepthPrePassCommandBuffer();
	}

	void createSwapChain();
	void createSwapChainImageViews();
	void createRenderPasses();
	void createDescriptorSetLayouts();
	void createGraphicsPipelines();
	void createCommandPool();
	void createDepthResources();
	void createFrameBuffers();
	void createTextureAndNormal();
	void createTextureSampler();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createLights();
	void createDescriptorPool();
	void createSceneObjectDescriptorSet();
	void createCameraDescriptorSet();
	void createIntermediateDescriptorSet();
	void updateIntermediateDescriptorSet();
	void createGraphicsCommandBuffers();
	void createSemaphores();

	void createComputePipeline();
	void createLigutCullingDescriptorSet();
	void createLightVisibilityBuffer();
	void createLightCullingCommandBuffer();

	void createDepthPrePassCommandBuffer();
	
	void updateUniformBuffers(float deltatime);
	void drawFrame();

	VRaii<VkShaderModule> createShaderModule(const std::vector<char>& code);


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

	std::tuple<VRaii<VkBuffer>, VRaii<VkDeviceMemory>> createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags property_bits, int sharing_queue_family_index_a = -1, int sharing_queue_family_index_b = -1);
	void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

	std::tuple<VRaii<VkImage>, VRaii<VkDeviceMemory>> createImage(uint32_t image_width, uint32_t image_height
		, VkFormat format, VkImageTiling tiling
		, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties);

	void copyImage(VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height);
	void transitImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);

	void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_mask, VkImageView* p_image_view);
	VRaii<VkImageView> createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_mask);

	std::tuple<VRaii<VkImage>, VRaii<VkDeviceMemory>, VRaii<VkImageView>> loadImageFromFile(std::string path);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	// Called on vulcan command buffer recording
	void recordCopyBuffer(VkCommandBuffer command_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
	void recordCopyImage(VkCommandBuffer command_buffer, VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height);
	void recordTransitImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);
};



_VulkanRenderer_Impl::_VulkanRenderer_Impl(GLFWwindow* window)
	:vulkan_context(window)
{

	queue_family_indices = vulkan_context.getQueueFamilyIndices();
	physical_device = vulkan_context.getPhysicalDevice();
	graphics_device = vulkan_context.getDevice();
	device = vulkan_context.getDevice();
	graphics_queue = vulkan_context.getGraphicsQueue();
	present_queue = vulkan_context.getPresentQueue();
	compute_queue = vulkan_context.getComputeQueue();

	std::tie(window_framebuffer_width, window_framebuffer_height) = vulkan_context.getWindowFrameBufferSize();
	
	initialize(); //TODO: allow multiple calls to initialize().... currently I am having problems with something like command buffers
}

void _VulkanRenderer_Impl::resize(int width, int height)
{
	if (width == 0 || height == 0) return;

	std::tie(window_framebuffer_width, window_framebuffer_height) = vulkan_context.getWindowFrameBufferSize();

	recreateSwapChain();
}

void _VulkanRenderer_Impl::requestDraw(float deltatime)
{
	updateUniformBuffers(deltatime); // TODO: there is graphics queue waiting in copyBuffer() called by this so I don't need to sync CPU and GPU elsewhere... but someday I will make the copy command able to use multiple times and I need to sync on writing the staging buffer
	drawFrame();
}

void _VulkanRenderer_Impl::cleanUp()
{
	vkDeviceWaitIdle(graphics_device);
}

void _VulkanRenderer_Impl::setCamera(const glm::mat4 & view, const glm::vec3 campos)
{
	view_matrix = view;
	this->cam_pos = campos;
}

void _VulkanRenderer_Impl::createSwapChain()
{
	auto support_details = SwapChainSupportDetails::querySwapChainSupport(physical_device, vulkan_context.getWindowSurface());

	VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(support_details.formats);
	VkPresentModeKHR present_mode = chooseSwapPresentMode(support_details.present_modes);
	VkExtent2D extent = chooseSwapExtent(support_details.capabilities);

	uint32_t queue_length = support_details.capabilities.minImageCount + 1;
	if (support_details.capabilities.maxImageCount > 0 && queue_length > support_details.capabilities.maxImageCount)
	{
		// 0 for maxImageCount means no limit
		queue_length = support_details.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = vulkan_context.getWindowSurface();
	create_info.minImageCount = queue_length;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1; // >1 when developing stereoscopic application
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // render directly
	// VK_IMAGE_USAGE_TRANSFER_DST_BIT and memory operation to enable post processing

	QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(physical_device, vulkan_context.getWindowSurface());
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphics_family, (uint32_t)indices.present_family };

	if (indices.graphics_family != indices.present_family)
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0; // Optional
		create_info.pQueueFamilyIndices = nullptr; // Optional
	}

	create_info.preTransform = support_details.capabilities.currentTransform; // not doing any transformation
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore alpha channel (for blending with other windows)

	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE; // ignore pixels obscured

	auto old_swap_chain = std::move(swap_chain); //which will be destroyed when out of scope
	create_info.oldSwapchain = old_swap_chain.get(); // required when recreating a swap chain (like resizing windows)

	VkSwapchainKHR tmp_swap_chain;
	vulkan_util::checkResult(vkCreateSwapchainKHR(graphics_device, &create_info, nullptr, &tmp_swap_chain), "Failed to create swap chain!");

	swap_chain = VRaii<vk::SwapchainKHR>(
			tmp_swap_chain,
			[device = this->device](auto & obj){device.destroySwapchainKHR(obj); }
	);

	uint32_t image_count;
	vkGetSwapchainImagesKHR(graphics_device, swap_chain.get(), &image_count, nullptr);
	swap_chain_images.resize(image_count);
	vkGetSwapchainImagesKHR(graphics_device, swap_chain.get(), &image_count, swap_chain_images.data());

	swap_chain_image_format = surface_format.format;
	swap_chain_extent = extent;
}

void _VulkanRenderer_Impl::createSwapChainImageViews()
{
	auto raii_deleter = [device = this->device](auto& obj)
	{
		device.destroyImageView(obj);
	};

	swap_chain_imageviews.clear(); // VRaii will delete old objects
	swap_chain_imageviews.reserve(swap_chain_images.size());

	for (uint32_t i = 0; i < swap_chain_images.size(); i++)
	{
		VkImageView tmp_imageview;
		createImageView(swap_chain_images[i], swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, &tmp_imageview);
		swap_chain_imageviews.emplace_back(tmp_imageview, raii_deleter);
	}
}

void _VulkanRenderer_Impl::createRenderPasses()
{
	auto renderpass_deletef = [device = this->device](auto & obj)
	{
		device.destroyRenderPass(obj);
	};

	// depth pre pass // TODO: should I merge this as a 
	{
		VkAttachmentDescription depth_attachment = {};
		depth_attachment.format = findDepthFormat();
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //TODO?
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // to be read in compute shader?

		VkAttachmentReference depth_attachment_ref = {};
		depth_attachment_ref.attachment = 0;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 0;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		// overwrite subpass dependency to make it wait until VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0; // 0  refers to the subpass
		dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 1> attachments = {  depth_attachment };

		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = (uint32_t)attachments.size();
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		VkRenderPass pass;
		if (vkCreateRenderPass(graphics_device, &render_pass_info, nullptr, &pass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create depth pre-pass!");
		}
		depth_pre_pass = VRaii<vk::RenderPass>(pass,renderpass_deletef);

	}
	// the render pass
	{
		VkAttachmentDescription color_attachment = {};
		color_attachment.format = swap_chain_image_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // before rendering
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // after rendering
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // no stencil
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // to be directly used in swap chain

		VkAttachmentDescription depth_attachment = {};
		depth_attachment.format = findDepthFormat();
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref = {};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		// overwrite subpass dependency to make it wait until VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0; // 0  refers to the subpass
		dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = (uint32_t)attachments.size();
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		VkRenderPass tmp_render_pass;
		if (vkCreateRenderPass(graphics_device, &render_pass_info, nullptr, &tmp_render_pass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
		render_pass = VRaii<vk::RenderPass>(tmp_render_pass, renderpass_deletef);

	}
}

void _VulkanRenderer_Impl::createDescriptorSetLayouts()
{
	auto raii_layout_deleter = [device = this->device](auto & layout)
	{
		device.destroyDescriptorSetLayout(layout);
	};

	// instance_descriptor_set_layout
	{
		// Transform information
		// create descriptor for uniform buffer objects
		VkDescriptorSetLayoutBinding ubo_layout_binding = {};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; 
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // only referencing from vertex shader
		// VK_SHADER_STAGE_ALL_GRAPHICS
		ubo_layout_binding.pImmutableSamplers = nullptr; // Optional

		// descriptor for texture sampler
		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 1;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.pImmutableSamplers = nullptr;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// descriptor for normal map sampler
		VkDescriptorSetLayoutBinding normalmap_layout_binding = {};
		normalmap_layout_binding.binding = 2;
		normalmap_layout_binding.descriptorCount = 1;
		normalmap_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalmap_layout_binding.pImmutableSamplers = nullptr;
		normalmap_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 3> bindings = { ubo_layout_binding, sampler_layout_binding, normalmap_layout_binding };
		// std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding};
		VkDescriptorSetLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = (uint32_t)bindings.size();
		layout_info.pBindings = bindings.data();


		object_descriptor_set_layout = VRaii<vk::DescriptorSetLayout>(
			device.createDescriptorSetLayout(layout_info, nullptr),
			raii_layout_deleter
		);
	}

	// camera_descriptor_set_layout
	{
		vk::DescriptorSetLayoutBinding ubo_layout_binding = {
			0,  // binding
			vk::DescriptorType::eStorageBuffer,  // descriptorType // FIXME: change back to uniform
			1,  // descriptorCount
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute, // stagFlags
			nullptr, // pImmutableSamplers
		};

		vk::DescriptorSetLayoutCreateInfo create_info = {
			vk::DescriptorSetLayoutCreateFlags(), // flags
			1,
			&ubo_layout_binding,
		};

		camera_descriptor_set_layout = VRaii<vk::DescriptorSetLayout>(
			device.createDescriptorSetLayout(create_info, nullptr),
			raii_layout_deleter
		);
	}

	// light_culling_descriptor_set_layout, shared between compute pipeline and graphics pipeline
	{
		std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {};

		{
			// create descriptor for storage buffer for light culling results
			VkDescriptorSetLayoutBinding lb = {};
			lb.binding = 0;
			lb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; 
			lb.descriptorCount = 1;
			lb.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			lb.pImmutableSamplers = nullptr;
			set_layout_bindings.push_back(lb);
		}

		{
			// uniform buffer for point lights
			VkDescriptorSetLayoutBinding lb = {};
			lb.binding = 1;
			lb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; // FIXME: change back to uniform
			lb.descriptorCount = 1;  // maybe we can use this for different types of lights
			lb.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			lb.pImmutableSamplers = nullptr;
			set_layout_bindings.push_back(lb);
		}

		VkDescriptorSetLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
		layout_info.pBindings = set_layout_bindings.data();

		light_culling_descriptor_set_layout = VRaii<vk::DescriptorSetLayout>(
			device.createDescriptorSetLayout(layout_info, nullptr),
			raii_layout_deleter
		);
	}

	// descriptor set layout for intermediate objects during render passes, such as z-buffer
	{
		// reads from depth attachment of previous frame
		// descriptor for texture sampler
		vk::DescriptorSetLayoutBinding sampler_layout_binding = {
			0, // binding
			vk::DescriptorType::eCombinedImageSampler, // descriptorType
			1, // descriptoCount
			vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment ,  //stageFlags 
			nullptr, // pImmutableSamplers
		};

		vk::DescriptorSetLayoutCreateInfo create_info = {
			vk::DescriptorSetLayoutCreateFlags(), // flags
			1,
			&sampler_layout_binding,
		};

		intermediate_descriptor_set_layout = VRaii<vk::DescriptorSetLayout>(
			device.createDescriptorSetLayout(create_info, nullptr),
			raii_layout_deleter
		);
	}
}


void _VulkanRenderer_Impl::createGraphicsPipelines()
{

	auto raii_pipeline_layout_deleter = [device = this->device](auto & obj)
	{
		device.destroyPipelineLayout(obj);
	};
	auto raii_pipeline_deleter = [device = this->device](auto & obj)
	{
		device.destroyPipeline(obj);
	};

	// create main pipeline
	{
		auto vert_shader_code = util::readFile(util::getContentPath("forwardplus_vert.spv"));
		auto frag_shader_code = util::readFile(util::getContentPath("forwardplus_frag.spv"));
		// auto light_culling_comp_shader_code = util::readFile(util::getContentPath("light_culling.comp.spv"));



		auto vert_shader_module = createShaderModule(vert_shader_code);
		auto frag_shader_module = createShaderModule(frag_shader_code);


		VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_shader_module.get();
		vert_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_shader_module.get();
		frag_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vert_shader_stage_info, frag_shader_stage_info };

		// vertex data info
		VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto binding_description = vulkan_util::getVertexBindingDesciption();
		auto attr_description = vulkan_util::getVertexAttributeDescriptions();

		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &binding_description;
		vertex_input_info.vertexAttributeDescriptionCount = (uint32_t)attr_description.size();
		vertex_input_info.pVertexAttributeDescriptions = attr_description.data(); // Optional

		// input assembler
		VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
		input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_info.primitiveRestartEnable = VK_FALSE;

		// viewport
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swap_chain_extent.width;
		viewport.height = (float)swap_chain_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swap_chain_extent;
		VkPipelineViewportStateCreateInfo viewport_state_info = {};
		viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state_info.viewportCount = 1;
		viewport_state_info.pViewports = &viewport;
		viewport_state_info.scissorCount = 1;
		viewport_state_info.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f; // requires wideLines feature enabled when larger than one
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		//rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // what
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // inverted Y during projection matrix
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// no multisampling
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; /// Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		// depth and stencil
		VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
		depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil.depthTestEnable = VK_TRUE;
		depth_stencil.depthWriteEnable = VK_TRUE;
		depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil.depthBoundsTestEnable = VK_FALSE;
		depth_stencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState color_blend_attachment = {};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		// Use alpha blending
		color_blend_attachment.blendEnable = VK_TRUE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blending_info = {};
		color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending_info.logicOpEnable = VK_FALSE;
		color_blending_info.logicOp = VK_LOGIC_OP_COPY; // Optional
		color_blending_info.attachmentCount = 1;
		color_blending_info.pAttachments = &color_blend_attachment;
		color_blending_info.blendConstants[0] = 0.0f; // Optional
		color_blending_info.blendConstants[1] = 0.0f; // Optional
		color_blending_info.blendConstants[2] = 0.0f; // Optional
		color_blending_info.blendConstants[3] = 0.0f; // Optional

		// parameters allowed to be changed without recreating a pipeline
		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
		dynamic_state_info.dynamicStateCount = 2;
		dynamic_state_info.pDynamicStates = dynamicStates;

		VkPushConstantRange push_constant_range = {};
		push_constant_range.offset = 0;
		push_constant_range.size = sizeof(PushConstantObject);
		push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// no uniform variables or push constants
		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkDescriptorSetLayout set_layouts[] = { object_descriptor_set_layout.get(), camera_descriptor_set_layout.get(), light_culling_descriptor_set_layout.get(), intermediate_descriptor_set_layout.get() };
		pipeline_layout_info.setLayoutCount = 4; // Optional
		pipeline_layout_info.pSetLayouts = set_layouts; // Optional
		pipeline_layout_info.pushConstantRangeCount = 1; // Optional
		pipeline_layout_info.pPushConstantRanges = &push_constant_range; // Optional

		VkPipelineLayout temp_layout;
		auto pipeline_layout_result = vkCreatePipelineLayout(graphics_device, &pipeline_layout_info, nullptr,
			&temp_layout);
		if (pipeline_layout_result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}
		pipeline_layout = VRaii<VkPipelineLayout>(temp_layout, raii_pipeline_layout_deleter);

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertex_input_info;
		pipelineInfo.pInputAssemblyState = &input_assembly_info;
		pipelineInfo.pViewportState = &viewport_state_info;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depth_stencil;
		pipelineInfo.pColorBlendState = &color_blending_info;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout = pipeline_layout.get();
		pipelineInfo.renderPass = render_pass.get();
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // not deriving from existing pipeline
		pipelineInfo.basePipelineIndex = -1; // Optional
		pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

		VkPipeline temp_pipeline;
		auto pipeline_result = vkCreateGraphicsPipelines(graphics_device, VK_NULL_HANDLE, 1
			, &pipelineInfo, nullptr, &temp_pipeline);
		graphics_pipeline = VRaii<VkPipeline>(temp_pipeline, raii_pipeline_deleter);

		if (pipeline_result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		//-------------------------------------depth prepass pipeline ------------------------------------------------


		auto depth_vert_shader_code = util::readFile(util::getContentPath("depth_vert.spv"));
		// auto light_culling_comp_shader_code = util::readFile(util::getContentPath("light_culling.comp.spv"));
		auto depth_vert_shader_module = createShaderModule(depth_vert_shader_code);
		VkPipelineShaderStageCreateInfo depth_vert_shader_stage_info = {};
		depth_vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		depth_vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		depth_vert_shader_stage_info.module = depth_vert_shader_module.get();
		depth_vert_shader_stage_info.pName = "main";
		VkPipelineShaderStageCreateInfo depth_shader_stages[] = { depth_vert_shader_stage_info };

		std::array<vk::DescriptorSetLayout, 2> depth_set_layouts = { object_descriptor_set_layout.get(), camera_descriptor_set_layout.get() };

		vk::PipelineLayoutCreateInfo depth_layout_info = {
			vk::PipelineLayoutCreateFlags(),  // flags
			static_cast<uint32_t>(depth_set_layouts.size()),  // setLayoutCount
			depth_set_layouts.data(),  // setlayouts
			0,  // pushConstantRangeCount
			nullptr // pushConstantRanges
		};
		depth_pipeline_layout = VRaii<vk::PipelineLayout>(
			device.createPipelineLayout(depth_layout_info, nullptr),
			raii_pipeline_layout_deleter
		);

		VkGraphicsPipelineCreateInfo depth_pipeline_info = {};
		depth_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		depth_pipeline_info.stageCount = 1;
		depth_pipeline_info.pStages = depth_shader_stages;

		depth_pipeline_info.pVertexInputState = &vertex_input_info;
		depth_pipeline_info.pInputAssemblyState = &input_assembly_info;
		depth_pipeline_info.pViewportState = &viewport_state_info;
		depth_pipeline_info.pRasterizationState = &rasterizer;
		depth_pipeline_info.pMultisampleState = &multisampling;
		depth_pipeline_info.pDepthStencilState = &depth_stencil;
		depth_pipeline_info.pColorBlendState = nullptr;
		depth_pipeline_info.pDynamicState = nullptr; // Optional
		depth_pipeline_info.layout = depth_pipeline_layout.get();
		depth_pipeline_info.renderPass = depth_pre_pass.get();
		depth_pipeline_info.subpass = 0;
		depth_pipeline_info.basePipelineHandle = graphics_pipeline.get(); // not deriving from existing pipeline
		depth_pipeline_info.basePipelineIndex = -1; // Optional
		depth_pipeline_info.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;

		depth_pipeline = VRaii<vk::Pipeline>(
			device.createGraphicsPipeline(vk::PipelineCache(), depth_pipeline_info, nullptr),
			raii_pipeline_deleter
		);
	}
}

void _VulkanRenderer_Impl::createFrameBuffers()
{
	auto raii_framebuffer_deleter = [device = this->device](auto & obj)
	{
		device.destroyFramebuffer(obj);
	};

	// swap chain frame buffers
	{
		swap_chain_framebuffers.clear(); // VDeleter will delete old objects
		swap_chain_framebuffers.reserve(swap_chain_imageviews.size());

		for (size_t i = 0; i < swap_chain_imageviews.size(); i++)
		{
			std::array<VkImageView, 2> attachments = { swap_chain_imageviews[i].get(), depth_image_view.get() };

			VkFramebufferCreateInfo framebuffer_info = {};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = render_pass.get();
			framebuffer_info.attachmentCount = (uint32_t)attachments.size();
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = swap_chain_extent.width;
			framebuffer_info.height = swap_chain_extent.height;
			framebuffer_info.layers = 1;

			swap_chain_framebuffers.emplace_back(
				device.createFramebuffer(framebuffer_info, nullptr),
				raii_framebuffer_deleter
			);
		}
	}

	// depth pass frame buffer
	{
		std::array<VkImageView, 1> attachments = { pre_pass_depth_image_view.get() };

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = depth_pre_pass.get();
		framebuffer_info.attachmentCount = (uint32_t)attachments.size();
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = swap_chain_extent.width;
		framebuffer_info.height = swap_chain_extent.height;
		framebuffer_info.layers = 1;

		depth_pre_pass_framebuffer = VRaii<vk::Framebuffer>(
			device.createFramebuffer(framebuffer_info, nullptr),
			raii_framebuffer_deleter
		);
	}
}

void _VulkanRenderer_Impl::createCommandPool()
{
	auto& indices = queue_family_indices;

	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = indices.graphics_family;
	pool_info.flags = 0; // Optional
	// hint the command pool will rerecord buffers by VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
	// allow buffers to be rerecorded individually by VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT

	command_pool = VRaii<vk::CommandPool>(
		device.createCommandPool(pool_info, nullptr),
		[device = this->device](auto& obj)
		{
			device.destroyCommandPool(obj);
		}
	);
}

void _VulkanRenderer_Impl::createDepthResources()
{
	VkFormat depth_format = findDepthFormat();
	std::tie(depth_image, depth_image_memory) = createImage(swap_chain_extent.width, swap_chain_extent.height
		, depth_format
		, VK_IMAGE_TILING_OPTIMAL
		, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT 
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	depth_image_view =  createImageView(depth_image.get(), depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
	transitImageLayout(depth_image.get(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// for depth pre pass and output as texture
	std::tie(pre_pass_depth_image, pre_pass_depth_image_memory) = createImage(swap_chain_extent.width, swap_chain_extent.height
		, depth_format
		, VK_IMAGE_TILING_OPTIMAL
		//, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT  // TODO: if creating another depth image for prepass use, use this only for rendering depth image
		, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	pre_pass_depth_image_view = createImageView(pre_pass_depth_image.get(), depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
	transitImageLayout(pre_pass_depth_image.get(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void _VulkanRenderer_Impl::createTextureAndNormal()
{
	std::tie(texture_image, texture_image_memory, texture_image_view) = loadImageFromFile(util::TEXTURE_PATH);
	std::tie(normalmap_image, normalmap_image_memory, normalmap_image_view) = loadImageFromFile(util::NORMALMAP_PATH);
}

void _VulkanRenderer_Impl::createTextureSampler()
{
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;

	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = 16;

	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;

	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;
	
	VkSampler sampler;
	if (vkCreateSampler(graphics_device, &sampler_info, nullptr, &sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler!");
	}

	texture_sampler = VRaii<VkSampler>(
		sampler,
		[device = this->device](auto& obj)
		{
			device.destroySampler(obj);
		}
	);
}

uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties, VkPhysicalDevice physical_device)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		bool type_supported = (type_filter & (1 << i)) != 0;
		bool properties_supported = ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties);
		if (type_supported && properties_supported)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type!");
}

void _VulkanRenderer_Impl::createVertexBuffer()
{
	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

	// create staging buffer
	VRaii<VkBuffer> staging_buffer;
	VRaii<VkDeviceMemory> staging_buffer_memory;
	std::tie(staging_buffer, staging_buffer_memory) = createBuffer(buffer_size
		, VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

	// copy data to staging buffer
	void* data;
	vkMapMemory(graphics_device, staging_buffer_memory.get(), 0, buffer_size, 0, &data); // access the graphics memory using mapping
		memcpy(data, vertices.data(), (size_t)buffer_size); // may not be immediate due to memory caching or write operation not visiable without VK_MEMORY_PROPERTY_HOST_COHERENT_BIT or explict flusing
	vkUnmapMemory(graphics_device, staging_buffer_memory.get());

	// create vertex buffer at optimized local memory which may not be directly accessable by memory mapping
	// as copy destination of staging buffer
	std::tie(vertex_buffer, vertex_buffer_memory) = createBuffer(buffer_size
		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// copy content of staging buffer to vertex buffer
	copyBuffer(staging_buffer.get(), vertex_buffer.get(), buffer_size);
}

void _VulkanRenderer_Impl::createIndexBuffer()
{
	VkDeviceSize buffer_size = sizeof(vertex_indices[0]) * vertex_indices.size();

	// create staging buffer
	VRaii<VkBuffer> staging_buffer;
	VRaii<VkDeviceMemory> staging_buffer_memory;
	std::tie(staging_buffer, staging_buffer_memory) = createBuffer(buffer_size
		, VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	vkMapMemory(graphics_device, staging_buffer_memory.get(), 0, buffer_size, 0, &data); // access the graphics memory using mapping
	memcpy(data, vertex_indices.data(), (size_t)buffer_size); // may not be immediate due to memory caching or write operation not visiable without VK_MEMORY_PROPERTY_HOST_COHERENT_BIT or explict flusing
	vkUnmapMemory(graphics_device, staging_buffer_memory.get());

	std::tie(index_buffer, index_buffer_memory) = createBuffer(buffer_size
		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// copy content of staging buffer to index buffer
	copyBuffer(staging_buffer.get(), index_buffer.get(), buffer_size);
}

void _VulkanRenderer_Impl::createUniformBuffers()
{
	// create buffers for scene object
	{
		VkDeviceSize bufferSize = sizeof(SceneObjectUbo);

		std::tie(object_staging_buffer, object_staging_buffer_memory) = createBuffer(bufferSize
			, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		std::tie(object_uniform_buffer, object_uniform_buffer_memory) = createBuffer(bufferSize
			, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	// Adding data to scene object buffer
	{
		SceneObjectUbo ubo = {};
		ubo.model = glm::mat4(1.0f);

		void* data;
		vkMapMemory(graphics_device, object_staging_buffer_memory.get(), 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(graphics_device, object_staging_buffer_memory.get());
		copyBuffer(object_staging_buffer.get(), object_uniform_buffer.get(), sizeof(ubo));
	}

	// create buffers for camera
	{
		VkDeviceSize bufferSize = sizeof(CameraUbo);

		std::tie(camera_staging_buffer, camera_staging_buffer_memory) = createBuffer(bufferSize
			, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		std::tie(camera_uniform_buffer, camera_uniform_buffer_memory) = createBuffer(bufferSize
			, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  // FIXME: change back to uniform
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			, queue_family_indices.graphics_family
			, queue_family_indices.compute_family);
	}
}

void _VulkanRenderer_Impl::createLights()
{
	for (int i = 0; i < 200; i++) {
		glm::vec3 color;
		do { color = { glm::linearRand(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)) }; } 
		while (color.length() < 0.8f);
		pointlights.emplace_back(glm::linearRand(LIGHTPOS_MIN, LIGHTPOS_MAX), 5, color);
	}
	// TODO: choose between memory mapping and staging buffer
	//  (given that the lights are moving)
	auto light_num = static_cast<int>(pointlights.size());

	pointlight_buffer_size = sizeof(PointLight) * MAX_POINT_LIGHT_COUNT + sizeof(int);
	pointlight_buffer_size = sizeof(PointLight) * MAX_POINT_LIGHT_COUNT + sizeof(glm::vec4); // vec4 rather than int for padding

	std::tie(lights_staging_buffer, lights_staging_buffer_memory) = createBuffer(pointlight_buffer_size
		, VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	std::tie(pointlight_buffer, pointlight_buffer_memory) = createBuffer(pointlight_buffer_size
		, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT  // FIXME: change back to uniform
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // using barrier to sync
}

void _VulkanRenderer_Impl::createDescriptorPool()
{
	// Create descriptor pool for uniform buffer
	std::array<VkDescriptorPoolSize, 3> pool_sizes = {};
	//std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = 4; // transform buffer & light buffer & camera buffer & light buffer in compute pipeline
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = 3; // sampler for color map and normal map and depth map from depth prepass
	pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[2].descriptorCount = 3; // light visiblity buffer in graphics pipeline and compute pipeline

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = (uint32_t)pool_sizes.size();
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = 4; 
	// TODO: one more in graphics pipeline for light visiblity
	pool_info.flags = 0;
	//poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	// TODO: use VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT so I can create a VKGemoetryClass

	VkDescriptorPool pool;
	if (vkCreateDescriptorPool(graphics_device, &pool_info, nullptr, &pool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool!");
	}

	descriptor_pool = VRaii<VkDescriptorPool>(
		pool,
		[device = this->device](auto& obj)
		{
			device.destroyDescriptorPool(obj);
		}
	);
}

void _VulkanRenderer_Impl::createSceneObjectDescriptorSet()
{
	
	VkDescriptorSetLayout layouts[] = { object_descriptor_set_layout.get() };
	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = descriptor_pool.get();
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(graphics_device, &alloc_info, &object_descriptor_set) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor set!");
	}

	// refer to the uniform object buffer
	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = object_uniform_buffer.get();
	buffer_info.offset = 0;
	buffer_info.range = sizeof(SceneObjectUbo);

	VkDescriptorImageInfo image_info = {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView = texture_image_view.get();
	image_info.sampler = texture_sampler.get();

	VkDescriptorImageInfo normalmap_info = {};
	normalmap_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalmap_info.imageView = normalmap_image_view.get();
	normalmap_info.sampler = texture_sampler.get();

	VkDescriptorBufferInfo lights_buffer_info = {};
	lights_buffer_info.buffer = pointlight_buffer.get();
	lights_buffer_info.offset = 0;
	lights_buffer_info.range = sizeof(PointLight) * MAX_POINT_LIGHT_COUNT + sizeof(int);

	//std::array<VkWriteDescriptorSet, 4> descriptor_writes = {};
	std::array<VkWriteDescriptorSet, 3> descriptor_writes = {};

	// ubo
	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = object_descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &buffer_info;
	descriptor_writes[0].pImageInfo = nullptr; // Optional
	descriptor_writes[0].pTexelBufferView = nullptr; // Optional

	// texture
	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = object_descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pImageInfo = &image_info;

	// normal map
	descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[2].dstSet = object_descriptor_set;
	descriptor_writes[2].dstBinding = 2;
	descriptor_writes[2].dstArrayElement = 0;
	descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[2].descriptorCount = 1;
	descriptor_writes[2].pImageInfo = &normalmap_info;

	vkUpdateDescriptorSets(graphics_device, (uint32_t)descriptor_writes.size()
		, descriptor_writes.data(), 0, nullptr);

}

void _VulkanRenderer_Impl::createCameraDescriptorSet()
{
	// Create descriptor set
	{
		vk::DescriptorSetAllocateInfo alloc_info = {
			descriptor_pool.get(),  // descriptorPool
			1,  // descriptorSetCount
			camera_descriptor_set_layout.data(), // pSetLayouts
		};

		camera_descriptor_set = device.allocateDescriptorSets(alloc_info)[0];
	}

	// Write desciptor set
	{
		// refer to the uniform object buffer
		vk::DescriptorBufferInfo camera_uniform_buffer_info{
			camera_uniform_buffer.get(), // buffer_ 
			0, //offset_
			sizeof(CameraUbo) // range_
		};

		std::vector<vk::WriteDescriptorSet> descriptor_writes = {};

		descriptor_writes.emplace_back(
			camera_descriptor_set, // dstSet
			0, // dstBinding
			0, // distArrayElement
			1, // descriptorCount
			vk::DescriptorType::eStorageBuffer, //descriptorType // FIXME: change back to uniform
			nullptr, //pImageInfo
			&camera_uniform_buffer_info, //pBufferInfo
			nullptr //pTexBufferView
		);

		std::array<vk::CopyDescriptorSet, 0> descriptor_copies;
		device.updateDescriptorSets(descriptor_writes, descriptor_copies);
	}
}

void _VulkanRenderer_Impl::createIntermediateDescriptorSet()
{
	// Create descriptor set
	{
		vk::DescriptorSetAllocateInfo alloc_info = {
			descriptor_pool.get(),  // descriptorPool
			1,  // descriptorSetCount
			intermediate_descriptor_set_layout.data(), // pSetLayouts
		};

		intermediate_descriptor_set = device.allocateDescriptorSets(alloc_info)[0];
	}

}

void _VulkanRenderer_Impl::updateIntermediateDescriptorSet()
{
	// Write desciptor set
	
		vk::DescriptorImageInfo depth_image_info = {
			texture_sampler.get(),
			pre_pass_depth_image_view.get(),
			vk::ImageLayout::eShaderReadOnlyOptimal
		};

		std::vector<vk::WriteDescriptorSet> descriptor_writes = {};

		descriptor_writes.emplace_back(
			intermediate_descriptor_set, // dstSet
			0, // dstBinding
			0, // distArrayElement
			1, // descriptorCount
			vk::DescriptorType::eCombinedImageSampler, //descriptorType // FIXME: change back to uniform
			&depth_image_info, //pImageInfo
			nullptr, //pBufferInfo
			nullptr //pTexBufferView
		);

		std::array<vk::CopyDescriptorSet, 0> descriptor_copies;
		device.updateDescriptorSets(descriptor_writes, descriptor_copies);
	
}

void _VulkanRenderer_Impl::createDepthPrePassCommandBuffer()
{
	if (depth_prepass_command_buffer)
	{
		device.freeCommandBuffers(command_pool.get(), 1, &depth_prepass_command_buffer);
		depth_prepass_command_buffer = VK_NULL_HANDLE;
	}

	// Create depth pre-pass command buffer
	{
		vk::CommandBufferAllocateInfo alloc_info = {
			command_pool.get(), // command pool
			vk::CommandBufferLevel::ePrimary, // level
			1 // commandBufferCount
		};

		depth_prepass_command_buffer = device.allocateCommandBuffers(alloc_info)[0];
	}

	// Begin command
	{
		vk::CommandBufferBeginInfo begin_info =
		{
			vk::CommandBufferUsageFlagBits::eSimultaneousUse,
			nullptr
		};

		auto command = depth_prepass_command_buffer;

		command.begin(begin_info);

		std::array<vk::ClearValue, 1> clear_values = {};
		clear_values[0].depthStencil = { 1.0f, 0 }; // 1.0 is far view plane
		vk::RenderPassBeginInfo depth_pass_info = {
			depth_pre_pass.get(),
			depth_pre_pass_framebuffer.get(),
			vk::Rect2D({ 0,0 }, swap_chain_extent),
			static_cast<uint32_t>(clear_values.size()),
			clear_values.data()
		};
		command.beginRenderPass(&depth_pass_info, vk::SubpassContents::eInline);
		command.bindPipeline(vk::PipelineBindPoint::eGraphics, depth_pipeline.get());

		std::array<vk::DescriptorSet, 2> depth_descriptor_sets = { object_descriptor_set, camera_descriptor_set };
		std::array<uint32_t, 0> depth_dynamic_offsets;
		command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, depth_pipeline_layout.get(), 0, depth_descriptor_sets, depth_dynamic_offsets);

		std::array<vk::Buffer, 1> depth_vertex_buffers = { vertex_buffer.get() };
		std::array<vk::DeviceSize, 1> depth_offsets = { 0 };
		command.bindVertexBuffers(0, depth_vertex_buffers, depth_offsets);
		command.bindIndexBuffer(index_buffer.get(), 0, vk::IndexType::eUint32);

		command.drawIndexed(static_cast<uint32_t>(vertex_indices.size()), 1, 0, 0, 0);

		command.endRenderPass();

		command.end();

	}

}

void _VulkanRenderer_Impl::createGraphicsCommandBuffers()
{
	// Free old command buffers, if any
	if (command_buffers.size() > 0)
	{
		vkFreeCommandBuffers(graphics_device, command_pool.get(), (uint32_t)command_buffers.size(), command_buffers.data());
	}
	command_buffers.clear();

	command_buffers.resize(swap_chain_framebuffers.size());

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool.get();
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	// primary: can be submitted to a queue but cannot be called from other command buffers
	// secondary: can be called by others but cannot be submitted to a queue
	alloc_info.commandBufferCount = (uint32_t)command_buffers.size();

	auto alloc_result = vkAllocateCommandBuffers(graphics_device, &alloc_info, command_buffers.data());
	if (alloc_result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	// record command buffers
	for (size_t i = 0; i < command_buffers.size(); i++)
	{
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		begin_info.pInheritanceInfo = nullptr; // Optional

		vkBeginCommandBuffer(command_buffers[i], &begin_info);

		// render pass
		{
			VkRenderPassBeginInfo render_pass_info = {};
			render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_info.renderPass = render_pass.get();
			render_pass_info.framebuffer = swap_chain_framebuffers[i].get();
			render_pass_info.renderArea.offset = { 0, 0 };
			render_pass_info.renderArea.extent = swap_chain_extent;

			std::array<VkClearValue, 2> clear_values = {};
			clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clear_values[1].depthStencil = { 1.0f, 0 }; // 1.0 is far view plane
			render_pass_info.clearValueCount = (uint32_t)clear_values.size();
			render_pass_info.pClearValues = clear_values.data();

			vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

			PushConstantObject pco = { 
				static_cast<int>(swap_chain_extent.width), 
				static_cast<int>(swap_chain_extent.height),
				tile_count_per_row, tile_count_per_col,
				debug_view_index
			};
			vkCmdPushConstants(command_buffers[i], pipeline_layout.get(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pco), &pco);

			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.get());

			// bind vertex buffer
			VkBuffer vertex_buffers[] = { vertex_buffer.get()};
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);
			//vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffer.get(), 0, VK_INDEX_TYPE_UINT32);

			std::array<VkDescriptorSet, 4> descriptor_sets = { object_descriptor_set, camera_descriptor_set, light_culling_descriptor_set, intermediate_descriptor_set };
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS
				, pipeline_layout.get(), 0, static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), 0, nullptr);
			// TODO: better to store vertex buffer and index buffer in a single VkBuffer


			//vkCmdDraw(command_buffers[i], VERTICES.size(), 1, 0, 0);
			vkCmdDrawIndexed(command_buffers[i], (uint32_t)vertex_indices.size(), 1, 0, 0, 0);

			vkCmdEndRenderPass(command_buffers[i]);
			recordTransitImageLayout(command_buffers[i], pre_pass_depth_image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		}

		auto record_result = vkEndCommandBuffer(command_buffers[i]);
		if (record_result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}
}

void _VulkanRenderer_Impl::createSemaphores()
{
	vk::SemaphoreCreateInfo semaphore_info = { vk::SemaphoreCreateFlags() };

	auto destroy_func = [&device = this->device](auto & obj)
	{
		device.destroySemaphore(obj);
	};

	render_finished_semaphore = VRaii<vk::Semaphore>(
		device.createSemaphore(semaphore_info, nullptr),
		destroy_func
	);
	image_available_semaphore = VRaii<vk::Semaphore>(
		device.createSemaphore(semaphore_info, nullptr),
		destroy_func
	);
	lightculling_completed_semaphore = VRaii<vk::Semaphore>(
		device.createSemaphore(semaphore_info, nullptr), 
		destroy_func
	);
	depth_prepass_finished_semaphore = VRaii<vk::Semaphore>(
		device.createSemaphore(semaphore_info, nullptr),
		destroy_func
	);
}


/** 
* Create compute pipeline for light culling
*/
void _VulkanRenderer_Impl::createComputePipeline()
{
	// TODO: I think I should have it as a member
	auto compute_queue_family_index = queue_family_indices.compute_family;

	// Step 1: Create Pipeline
	{
		auto raii_pipeline_layout_deleter = [device = this->device](auto & obj)
		{
			device.destroyPipelineLayout(obj);
		};
		auto raii_pipeline_deleter = [device = this->device](auto & obj)
		{
			device.destroyPipeline(obj);
		};

		VkPushConstantRange push_constant_range = {};
		push_constant_range.offset = 0;
		push_constant_range.size = sizeof(PushConstantObject);
		push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		std::array<VkDescriptorSetLayout, 3> set_layouts = { light_culling_descriptor_set_layout.get(), camera_descriptor_set_layout.get(), intermediate_descriptor_set_layout.get()};
		pipeline_layout_info.setLayoutCount = static_cast<int>(set_layouts.size()); 
		pipeline_layout_info.pSetLayouts = set_layouts.data(); 
		pipeline_layout_info.pushConstantRangeCount = 1; 
		pipeline_layout_info.pPushConstantRanges = &push_constant_range; 

		VkPipelineLayout temp_layout;
		vulkan_util::checkResult(vkCreatePipelineLayout(graphics_device, &pipeline_layout_info, nullptr, &temp_layout));
		compute_pipeline_layout = VRaii<VkPipelineLayout>(temp_layout, raii_pipeline_layout_deleter);

		auto light_culling_comp_shader_code = util::readFile(util::getContentPath("light_culling_comp.spv"));
		
		auto comp_shader_module = createShaderModule(light_culling_comp_shader_code);
		VkPipelineShaderStageCreateInfo comp_shader_stage_info = {};
		comp_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		comp_shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		comp_shader_stage_info.module = comp_shader_module.get();
		comp_shader_stage_info.pName = "main";
		
		VkComputePipelineCreateInfo pipeline_create_info;
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_create_info.stage = comp_shader_stage_info;
		pipeline_create_info.layout = compute_pipeline_layout.get();
		pipeline_create_info.pNext = nullptr;
		pipeline_create_info.flags = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE; // not deriving from existing pipeline
		pipeline_create_info.basePipelineIndex = -1; // Optional

		VkPipeline temp_pipeline;
		vulkan_util::checkResult(vkCreateComputePipelines(graphics_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &temp_pipeline));
		compute_pipeline = VRaii<VkPipeline>(temp_pipeline, raii_pipeline_deleter);
	};

	// Step 2: create compute command pool
	{
		VkCommandPoolCreateInfo cmd_pool_info = {};
		cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_pool_info.queueFamilyIndex = compute_queue_family_index;
		cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;


		compute_command_pool = VRaii<vk::CommandPool>(
			device.createCommandPool(cmd_pool_info, nullptr),
			[device = this->device](auto& obj)
			{
				device.destroyCommandPool(obj);
			}
		);
	}
}

/**
* creating light visiblity descriptor sets for both passes
*/
void _VulkanRenderer_Impl::createLigutCullingDescriptorSet()
{
	// create shared dercriptor set between compute pipeline and rendering pipeline
	{	
		// todo: reduce code duplication with createDescriptorSet() 
		VkDescriptorSetLayout layouts[] = { light_culling_descriptor_set_layout.get() };
		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = descriptor_pool.get();
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = layouts;

		light_culling_descriptor_set = device.allocateDescriptorSets(alloc_info)[0];
	}

}

// just for sizing information
struct _Dummy_VisibleLightsForTile
{
	uint32_t count;
	std::array<uint32_t, MAX_POINT_LIGHT_PER_TILE> lightindices;
};

/**
* Create or recreate light visibility buffer and its descriptor
*/
void _VulkanRenderer_Impl::createLightVisibilityBuffer()
{
	assert(sizeof(_Dummy_VisibleLightsForTile) == sizeof(int) * (MAX_POINT_LIGHT_PER_TILE + 1));

	tile_count_per_row = (swap_chain_extent.width - 1) / TILE_SIZE + 1;
	tile_count_per_col = (swap_chain_extent.height - 1) / TILE_SIZE + 1;

	light_visibility_buffer_size = sizeof(_Dummy_VisibleLightsForTile) * tile_count_per_row * tile_count_per_col;

	std::tie(light_visibility_buffer, light_visibility_buffer_memory) = createBuffer(
		light_visibility_buffer_size
		, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	); // using barrier to sync

	// Write desciptor set in compute shader
	{
		// refer to the uniform object buffer
		vk::DescriptorBufferInfo light_visibility_buffer_info{
			light_visibility_buffer.get(), // buffer_ 
			0, //offset_
			light_visibility_buffer_size // range_
		};

		// refer to the uniform object buffer
		vk::DescriptorBufferInfo pointlight_buffer_info = { 
			pointlight_buffer.get(), // buffer_ 
			0, //offset_
			pointlight_buffer_size // range_
		};

		std::vector<vk::WriteDescriptorSet> descriptor_writes = {};
		
		descriptor_writes.emplace_back(
			light_culling_descriptor_set, // dstSet
			0, // dstBinding
			0, // distArrayElement
			1, // descriptorCount
			vk::DescriptorType::eStorageBuffer, //descriptorType
			nullptr, //pImageInfo
			&light_visibility_buffer_info, //pBufferInfo
			nullptr //pTexBufferView
		);

		descriptor_writes.emplace_back(
			light_culling_descriptor_set, // dstSet
			1, // dstBinding
			0, // distArrayElement
			1, // descriptorCount
			vk::DescriptorType::eStorageBuffer, //descriptorType // FIXME: change back to uniform
			nullptr, //pImageInfo
			&pointlight_buffer_info, //pBufferInfo
			nullptr //pTexBufferView
		);

		std::array<vk::CopyDescriptorSet, 0> descriptor_copies;
		device.updateDescriptorSets(descriptor_writes, descriptor_copies);
	}

}

void _VulkanRenderer_Impl::createLightCullingCommandBuffer()
{
	
	if (light_culling_command_buffer)
	{
		device.freeCommandBuffers(compute_command_pool.get(), 1, &light_culling_command_buffer);
		light_culling_command_buffer = VK_NULL_HANDLE;
	}

	// Create light culling command buffer
	{
		vk::CommandBufferAllocateInfo alloc_info = {
			compute_command_pool.get(), // command pool
			vk::CommandBufferLevel::ePrimary, // level
			1 // commandBufferCount
		};

		light_culling_command_buffer = device.allocateCommandBuffers(alloc_info)[0];
	}

	// Record command buffer
	{
		vk::CommandBufferBeginInfo begin_info =
		{
			vk::CommandBufferUsageFlagBits::eSimultaneousUse,
			nullptr
		};

		vk::CommandBuffer command(light_culling_command_buffer);

		command.begin(begin_info);

		// using barrier since the sharing mode when allocating memory is exclusive
		// begin after fragment shader finished reading from storage buffer
		
		std::vector<vk::BufferMemoryBarrier> barriers_before;
		barriers_before.emplace_back
		(
			vk::AccessFlagBits::eShaderRead,  // srcAccessMask
			vk::AccessFlagBits::eShaderWrite,  // dstAccessMask
			static_cast<uint32_t>(queue_family_indices.graphics_family),  // srcQueueFamilyIndex
			static_cast<uint32_t>(queue_family_indices.compute_family),  // dstQueueFamilyIndex
			static_cast<vk::Buffer>(light_visibility_buffer.get()),  // buffer
			0,  // offset
			light_visibility_buffer_size  // size
		);
		barriers_before.emplace_back
		(
			vk::AccessFlagBits::eShaderRead,  // srcAccessMask // FIXME: change back to uniform
			vk::AccessFlagBits::eShaderWrite,  // dstAccessMask
			static_cast<uint32_t>(queue_family_indices.graphics_family),  // srcQueueFamilyIndex
			static_cast<uint32_t>(queue_family_indices.compute_family),  // dstQueueFamilyIndex
			static_cast<vk::Buffer>(pointlight_buffer.get()),  // buffer
			0,  // offset
			pointlight_buffer_size  // size
		);

		command.pipelineBarrier(
			vk::PipelineStageFlagBits::eFragmentShader,  // srcStageMask
			vk::PipelineStageFlagBits::eComputeShader,  // dstStageMask
			vk::DependencyFlags(),  // dependencyFlags
			0,  // memoryBarrierCount 
			nullptr,  // pBUfferMemoryBarriers
			barriers_before.size(),  // bufferMemoryBarrierCount
			barriers_before.data(),  // pBUfferMemoryBarriers
			0,  // imageMemoryBarrierCount
			nullptr // pImageMemoryBarriers
		);


		// barrier
		command.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute, // pipelineBindPoint
			compute_pipeline_layout.get(), // layout
			0, // firstSet
			std::array<vk::DescriptorSet, 3>{light_culling_descriptor_set, camera_descriptor_set, intermediate_descriptor_set}, // descriptorSets
			std::array<uint32_t, 0>() // pDynamicOffsets
		); 

		PushConstantObject pco = { static_cast<int>(swap_chain_extent.width), static_cast<int>(swap_chain_extent.height), tile_count_per_row, tile_count_per_col };
		command.pushConstants(compute_pipeline_layout.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pco), &pco);

		command.bindPipeline(vk::PipelineBindPoint::eCompute, static_cast<VkPipeline>(compute_pipeline.get()));
		command.dispatch(tile_count_per_row, tile_count_per_col, 1);


		std::vector<vk::BufferMemoryBarrier> barriers_after;
		barriers_after.emplace_back
		(
			vk::AccessFlagBits::eShaderWrite,  // srcAccessMask
			vk::AccessFlagBits::eShaderRead,  // dstAccessMask
			static_cast<uint32_t>(queue_family_indices.compute_family), // srcQueueFamilyIndex
			static_cast<uint32_t>(queue_family_indices.graphics_family),  // dstQueueFamilyIndex
			static_cast<vk::Buffer>(light_visibility_buffer.get()),  // buffer
			0,  // offset
			light_visibility_buffer_size  // size
		);
		barriers_after.emplace_back
		(
			vk::AccessFlagBits::eShaderWrite,  // srcAccessMask // TODO: change back to uniform
			vk::AccessFlagBits::eShaderRead,  // dstAccessMask
			static_cast<uint32_t>(queue_family_indices.compute_family), // srcQueueFamilyIndex
			static_cast<uint32_t>(queue_family_indices.graphics_family),  // dstQueueFamilyIndex
			static_cast<vk::Buffer>(pointlight_buffer.get()),  // buffer
			0,  // offset
			pointlight_buffer_size  // size
		);

		command.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlags(),
			0, nullptr,
			barriers_after.size(), barriers_after.data(), // TODO
			0, nullptr
		);

		command.end();
	}
}



void _VulkanRenderer_Impl::updateUniformBuffers(float deltatime)
{
	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0f;

	// update camera ubo
	{
		CameraUbo ubo = {};
		ubo.view = view_matrix;
		ubo.proj = glm::perspective(glm::radians(45.0f), swap_chain_extent.width / (float)swap_chain_extent.height, 0.5f, 100.0f);
		ubo.proj[1][1] *= -1; //since the Y axis of Vulkan NDC points down
		ubo.projview = ubo.proj * ubo.view;
		ubo.cam_pos = cam_pos;

		void* data;
		vkMapMemory(graphics_device, camera_staging_buffer_memory.get(), 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(graphics_device, camera_staging_buffer_memory.get());

		// TODO: maybe I shouldn't use single time buffer
		copyBuffer(camera_staging_buffer.get(), camera_uniform_buffer.get(), sizeof(ubo));
	}

	// update light ubo
	{
		auto light_num = static_cast<int>(pointlights.size());
		VkDeviceSize bufferSize = sizeof(PointLight) * MAX_POINT_LIGHT_COUNT + sizeof(int);

		for (int i = 0; i < light_num; i++) {
			pointlights[i].pos += glm::vec3(0, 3.0f, 0) * deltatime;
			if (pointlights[i].pos.y > LIGHTPOS_MAX.y) {
				pointlights[i].pos.y -= (LIGHTPOS_MAX.y - LIGHTPOS_MIN.y);
			}
		}

		auto pointlights_size = sizeof(PointLight) * pointlights.size();
		void* data;
		vkMapMemory(graphics_device, lights_staging_buffer_memory.get(), 0, pointlight_buffer_size, 0, &data);
		memcpy(data, &light_num, sizeof(int));
		memcpy((char*)data + sizeof(glm::vec4), pointlights.data(), pointlights_size);
		vkUnmapMemory(graphics_device, lights_staging_buffer_memory.get());
		copyBuffer(lights_staging_buffer.get(), pointlight_buffer.get(), pointlight_buffer_size);
	}
}

const uint64_t ACQUIRE_NEXT_IMAGE_TIMEOUT{ std::numeric_limits<uint64_t>::max() };

void _VulkanRenderer_Impl::drawFrame()
{
	// 1. Acquiring an image from the swap chain
	uint32_t image_index;
	{
		auto aquiring_result = vkAcquireNextImageKHR(graphics_device, swap_chain.get()
			, ACQUIRE_NEXT_IMAGE_TIMEOUT, image_available_semaphore.get(), VK_NULL_HANDLE, &image_index);

		if (aquiring_result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// when swap chain needs recreation
			recreateSwapChain();
			return;
		}
		else if (aquiring_result != VK_SUCCESS && aquiring_result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}
	}

	// submit depth pre-pass command buffer
	{
		vk::SubmitInfo submit_info = {
			0, // waitSemaphoreCount
			nullptr, // pWaitSemaphores
			nullptr, // pwaitDstStageMask
			1, // commandBufferCount
			&depth_prepass_command_buffer, // pCommandBuffers
			1, // singalSemaphoreCount
			depth_prepass_finished_semaphore.data() // pSingalSemaphores
		};
		graphics_queue.submit(1, &submit_info, VK_NULL_HANDLE);
	}

	// submit light culling command buffer
	{
		vk::Semaphore wait_semaphores[] = { depth_prepass_finished_semaphore.get() }; // which semaphore to wait
		vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eComputeShader }; // which stage to execute
		vk::SubmitInfo submit_info = {
			1, // waitSemaphoreCount
			wait_semaphores, // pWaitSemaphores
			wait_stages, // pwaitDstStageMask
			1, // commandBufferCount
			&light_culling_command_buffer, // pCommandBuffers
			1, // singalSemaphoreCount
			lightculling_completed_semaphore.data() // pSingalSemaphores
		};
		compute_queue.submit(1, &submit_info, VK_NULL_HANDLE);
	}

	// 2. Submitting the command buffer
	{
		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore wait_semaphores[] = { image_available_semaphore.get() , lightculling_completed_semaphore.get() }; // which semaphore to wait
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT }; // which stage to execute
		submit_info.waitSemaphoreCount = 2;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffers[image_index];
		VkSemaphore signal_semaphores[] = { render_finished_semaphore.get() };
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_semaphores;
	
		auto submit_result = vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
		if (submit_result != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer!");
		}
	}
	// TODO: use Fence and we can have cpu start working at a earlier time

	// 3. Submitting the result back to the swap chain to show it on screen
	{
		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		VkSemaphore present_wait_semaphores[] = { render_finished_semaphore.get() };
		present_info.pWaitSemaphores = present_wait_semaphores;
		VkSwapchainKHR swapChains[] = { swap_chain.get() };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapChains;
		present_info.pImageIndices = &image_index;
		present_info.pResults = nullptr; // Optional, check for if every single chains is successful

		VkResult present_result = vkQueuePresentKHR(present_queue, &present_info);

		if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
		{
			recreateSwapChain();
		}
		else if (present_result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}
	}
}

VRaii<VkShaderModule> _VulkanRenderer_Impl::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = (uint32_t*)code.data();

	VkShaderModule temp_sm;
	auto result = vkCreateShaderModule(graphics_device, &create_info, nullptr, &temp_sm);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}
	return VRaii<VkShaderModule>(
		temp_sm,
		[device = this->device](auto& obj)
		{
			device.destroyShaderModule(obj);
		}
	);
}

VkSurfaceFormatKHR _VulkanRenderer_Impl::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	// When free to choose format
	if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& available_format : available_formats)
	{
	    // prefer 32bits RGBA color with SRGB support
		if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return available_format;
		}
	}

	// TODO: Rank how good the formats are and choose the best?

	return available_formats[0];
}

VkPresentModeKHR _VulkanRenderer_Impl::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
	for (const auto& available_present_mode : available_present_modes)
	{
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return available_present_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D _VulkanRenderer_Impl::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	// The swap extent is the resolution of the swap chain images
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actual_extent = { (uint32_t)window_framebuffer_width, (uint32_t)window_framebuffer_height };

		actual_extent.width = std::max(capabilities.minImageExtent.width
			, std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height
			, std::min(capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}

VkFormat _VulkanRenderer_Impl::findSupportedFormat(const std::vector<VkFormat>& candidates
	, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!");
}

std::tuple<VRaii<VkBuffer>, VRaii<VkDeviceMemory>> _VulkanRenderer_Impl::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags property_bits
	, int sharing_queue_family_index_a, int sharing_queue_family_index_b)
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;

	std::array<uint32_t, 2> indices;
	if (sharing_queue_family_index_a >= 0 && sharing_queue_family_index_b >= 0 && sharing_queue_family_index_a != sharing_queue_family_index_b)
	{
		indices = { static_cast<uint32_t>(sharing_queue_family_index_a) , static_cast<uint32_t>(sharing_queue_family_index_b) };
		buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		buffer_info.queueFamilyIndexCount = indices.size();
		buffer_info.pQueueFamilyIndices = indices.data();
	}
	else
	{
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; 
	}
	buffer_info.flags = 0;

	VkBuffer buffer;
	auto buffer_result = vkCreateBuffer(graphics_device, &buffer_info, nullptr, &buffer);

	if (buffer_result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create buffer!");
	}

	// allocate memory for buffer
	VkMemoryRequirements memory_req;
	vkGetBufferMemoryRequirements(graphics_device, buffer, &memory_req);

	VkMemoryAllocateInfo memory_alloc_info = {};
	memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_alloc_info.allocationSize = memory_req.size;
	memory_alloc_info.memoryTypeIndex = findMemoryType(memory_req.memoryTypeBits
		, property_bits
		, physical_device);

	VkDeviceMemory buffer_memory;
	auto memory_result = vkAllocateMemory(graphics_device, &memory_alloc_info, nullptr, &buffer_memory);
	if (memory_result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	// bind buffer with memory
	auto bind_result = vkBindBufferMemory(graphics_device, buffer, buffer_memory, 0);
	if (bind_result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to bind buffer memory!");
	}

	auto raii_buffer_deleter = [device = this->device](auto& obj)
	{
		device.destroyBuffer(obj);
	};

	auto raii_memory_deleter = [device = this->device](auto& obj)
	{
		device.freeMemory(obj);
	};

	return std::make_tuple(VRaii<VkBuffer>(buffer, raii_buffer_deleter)
		, VRaii<VkDeviceMemory>(buffer_memory, raii_memory_deleter));
}

void _VulkanRenderer_Impl::copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
	VkCommandBuffer copy_command_buffer = beginSingleTimeCommands();

	recordCopyBuffer(copy_command_buffer, src_buffer, dst_buffer, size);

	endSingleTimeCommands(copy_command_buffer);
}

std::tuple<VRaii<VkImage>, VRaii<VkDeviceMemory>> _VulkanRenderer_Impl::createImage(uint32_t image_width, uint32_t image_height
	, VkFormat format, VkImageTiling tiling
	, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties)
{
	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = image_width;
	image_info.extent.height = image_height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;

	image_info.format = format; //VK_FORMAT_R8G8B8A8_UNORM;
	image_info.tiling = tiling; //VK_IMAGE_TILING_LINEAR;
								// VK_IMAGE_TILING_OPTIMAL is better if don't need to directly access memory
	image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED; // VK_IMAGE_LAYOUT_UNDEFINED for attachments like color/depth buffer

	image_info.usage = usage; //VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // one queue family only
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0; // there are flags for sparse image (well not the case)


	VkImage vkimage;
	if (vkCreateImage(graphics_device, &image_info, nullptr, &vkimage) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	// allocate image memory
	VkMemoryRequirements memory_req;
	vkGetImageMemoryRequirements(graphics_device, vkimage, &memory_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_req.size;
	alloc_info.memoryTypeIndex = findMemoryType(memory_req.memoryTypeBits
		, memory_properties
		, physical_device);

	VkDeviceMemory memory;
	if (vkAllocateMemory(graphics_device, &alloc_info, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(graphics_device, vkimage, memory, 0);

	auto raii_image_deleter = [device = this->device](auto& obj)
	{
		device.destroyImage(obj);
	};
	
	auto raii_memory_deleter = [device = this->device](auto& obj)
	{
		device.freeMemory(obj);
	};
	
	return std::make_tuple(VRaii<VkImage>(vkimage, raii_image_deleter), VRaii<VkDeviceMemory>(memory, raii_memory_deleter));
}

void _VulkanRenderer_Impl::copyImage(VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height)
{
	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	recordCopyImage(command_buffer, src_image, dst_image, width, height);

	endSingleTimeCommands(command_buffer);
}

void _VulkanRenderer_Impl::transitImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	recordTransitImageLayout(command_buffer, image, old_layout, new_layout);

	endSingleTimeCommands(command_buffer);

}

void _VulkanRenderer_Impl::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_mask, VkImageView* p_image_view)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;

	// no swizzle
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	viewInfo.subresourceRange.aspectMask = aspect_mask;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(graphics_device, &viewInfo, nullptr, p_image_view) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image view!");
	}
}


VRaii<VkImageView> _VulkanRenderer_Impl::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_mask)
{
	VkImageView img_view;
	createImageView(image, format, aspect_mask, &img_view);
	return VRaii<VkImageView>(img_view, [device = this->device](auto& obj) {device.destroyImageView(obj); });
}

std::tuple<VRaii<VkImage>, VRaii<VkDeviceMemory>, VRaii<VkImageView>> _VulkanRenderer_Impl::loadImageFromFile(std::string path)
{
	// TODO: maybe move to vulkan_util or a VulkanDevice class

	// load image file
	int tex_width, tex_height, tex_channels;

	stbi_uc * pixels = stbi_load(path.c_str()
		, &tex_width, &tex_height
		, &tex_channels
		, STBI_rgb_alpha);

	VkDeviceSize image_size = tex_width * tex_height * 4;
	if (!pixels)
	{
		throw std::runtime_error("Failed to load image" + path);
	}

	// create staging image memory
	VRaii<VkImage> staging_image;
	VRaii<VkDeviceMemory> staging_image_memory;
	std::tie(staging_image, staging_image_memory) = createImage(
		tex_width, tex_height
		, VK_FORMAT_R8G8B8A8_UNORM
		, VK_IMAGE_TILING_LINEAR
		, VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	// copy image to staging memory
	void* data;
	vkMapMemory(graphics_device, staging_image_memory.get(), 0, image_size, 0, &data);
	memcpy(data, pixels, (size_t)image_size);
	vkUnmapMemory(graphics_device, staging_image_memory.get());

	// free image in memory
	stbi_image_free(pixels);

	VRaii<VkImage> image;
	VRaii<VkDeviceMemory> image_memory;
	// create texture image
	std::tie(image, image_memory) = createImage(
		tex_width, tex_height
		, VK_FORMAT_R8G8B8A8_UNORM
		, VK_IMAGE_TILING_OPTIMAL
		, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	// TODO: doing the steps asynchronously by using a single command buffer
	auto command_buffer = beginSingleTimeCommands();

	recordTransitImageLayout(command_buffer, staging_image.get(), VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	recordTransitImageLayout(command_buffer, image.get(), VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	recordCopyImage(command_buffer, staging_image.get(), image.get(), tex_width, tex_height);
	recordTransitImageLayout(command_buffer, image.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	endSingleTimeCommands(command_buffer);


	// Create image view
	auto image_view = createImageView(image.get(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	return std::make_tuple(std::move(image), std::move(image_memory), std::move(image_view));
}

// create a temperorary command buffer for one-time use
// and begin recording
VkCommandBuffer _VulkanRenderer_Impl::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool.get();
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(graphics_device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

// End recording the single time command, submit then wait for execution and destroy the buffer
void _VulkanRenderer_Impl::endSingleTimeCommands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	// execute the command buffer and wait for the execution
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;
	vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	// free the temperorary command buffer
	vkFreeCommandBuffers(graphics_device, command_pool.get(), 1, &command_buffer);
}

void _VulkanRenderer_Impl::recordCopyBuffer(VkCommandBuffer command_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
	VkBufferCopy copy_region = {};
	copy_region.srcOffset = 0; // Optional
	copy_region.dstOffset = 0; // Optional
	copy_region.size = size;

	vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
}

void _VulkanRenderer_Impl::recordCopyImage(VkCommandBuffer command_buffer, VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height)
{
	VkImageSubresourceLayers sub_besource = {};
	sub_besource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	sub_besource.baseArrayLayer = 0;
	sub_besource.mipLevel = 0;
	sub_besource.layerCount = 1;

	VkImageCopy region = {};
	region.srcSubresource = sub_besource;
	region.dstSubresource = sub_besource;
	region.srcOffset = { 0, 0, 0 };
	region.dstOffset = { 0, 0, 0 };
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vkCmdCopyImage(command_buffer,
		src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region
		);
}

void _VulkanRenderer_Impl::recordTransitImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
	// barrier is used to ensure a buffer has finished writing before
	// reading as weel as doing transition
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  //TODO: when converting the depth attachment for depth pre pass this is not correct
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || old_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		// dst must wait on src
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
			| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
			| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
			| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(command_buffer
		, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT // which stage to happen before barrier
		, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT // which stage needs to wait for barrier
		, 0
		, 0, nullptr
		, 0, nullptr
		, 1, &barrier
		);
}

VulkanRenderer::VulkanRenderer(GLFWwindow * window)
	:p_impl(std::make_unique<_VulkanRenderer_Impl>(window))
{}

VulkanRenderer::~VulkanRenderer() = default;

int VulkanRenderer::getDebugViewIndex() const
{
	return p_impl->getDebugViewIndex();
}

void VulkanRenderer::resize(int width, int height)
{
	p_impl->resize(width, height);
}

void VulkanRenderer::changeDebugViewIndex(int target_view)
{
	p_impl->changeDebugViewIndex(target_view);
}

void VulkanRenderer::requestDraw(float deltatime)
{
	p_impl->requestDraw(deltatime);
}

void VulkanRenderer::cleanUp()
{
	p_impl->cleanUp();
}

void VulkanRenderer::setCamera(const glm::mat4 & view, const glm::vec3 campos)
{
	p_impl->setCamera(view, campos);
}
