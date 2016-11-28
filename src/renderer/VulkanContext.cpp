// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "VulkanContext.h"

#include "vulkan_util.h"

#include <glm/gtc/matrix_transform.hpp>

#include <stb_image.h>

#include <functional>
#include <vector>
#include <unordered_set>
#include <string>
#include <cstring>
#include <set>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <iostream>

using util::Vertex;


struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;

	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		// Getting supported surface formats
		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
		if (format_count != 0)
		{
			details.formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
		}

		// Getting supported present modes
		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
		if (present_mode_count != 0)
		{
			details.present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
		}

		return details;
	}
};

QueueFamilyIndices QueueFamilyIndices::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t queuefamily_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queuefamily_count, nullptr);

	std::vector<VkQueueFamilyProperties> queuefamilies(queuefamily_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queuefamily_count, queuefamilies.data());

	int i = 0;
	for (const auto& queuefamily : queuefamilies)
	{
		if (queuefamily.queueCount > 0 && queuefamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			// Graphics queue_family
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (queuefamily.queueCount > 0 && presentSupport)
		{
			// Graphics queue_family
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}


	return indices;
}

VulkanContext::VulkanContext(GLFWwindow* window)
{
	if (!window)
	{
		throw std::runtime_error("invalid window");
	}

	glfwGetFramebufferSize(window, &window_framebuffer_width, &window_framebuffer_height);

	this->window = window;

	initVulkan();
}

VkResult VulkanContext::CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanContext::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

// the debug callback function that Vulkan runs
VkBool32 debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{

	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

void VulkanContext::resize(int width, int height)
{
	if (width == 0 || height == 0) return;

	recreateSwapChain();
}

void VulkanContext::requestDraw()
{
	updateUniformBuffer(); // TODO: there is graphics queue waiting in copyBuffer() called by this so I don't need to sync CPU and GPU elsewhere... but someday I will make the copy command able to use multiple times and I need to sync on writing the staging buffer
	drawFrame();
}

void VulkanContext::cleanUp()
{
	vkDeviceWaitIdle(graphics_device);
}

void VulkanContext::setViewMatrix(const glm::mat4 & view)
{
	view_matrix = view;
}

void VulkanContext::initVulkan()
{
	createInstance();
	setupDebugCallback();
	createWindowSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createSwapChainImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createDepthResources();
	createFrameBuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	std::tie(vertices, vertex_indices) = util::loadModel();
	// TODO: better to use a single memory allocation for multiple buffers
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffer();
	createLights();
	createDescriptorPool();
	createDescriptorSet();
	createCommandBuffers();
	createSemaphores();
}

// Needs to be called right after instance creation because it may influence device selection
void VulkanContext::createWindowSurface()
{
	auto result = glfwCreateWindowSurface(instance, window, nullptr, &window_surface);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface!");
	}
}

void VulkanContext::recreateSwapChain()
{
	vkDeviceWaitIdle(graphics_device);

	createSwapChain();
	createSwapChainImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFrameBuffers();
	createCommandBuffers();
}

void VulkanContext::createInstance()
{
	if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}


	VkApplicationInfo app_info = {}; // optional
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vulkan Hello World";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instance_info = {}; // not optional
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;

	// Getting Vulkan instance extensions required by GLFW
	auto glfwExtensions = getRequiredExtensions();

	// Getting Vulkan supported extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	std::unordered_set<std::string> supportedExtensionNames;
	for (const auto& extension : extensions)
	{
		supportedExtensionNames.insert(std::string(extension.extensionName));
	}

	// Print Vulkan supported extensions
	std::cout << "available extensions:" << std::endl;
	for (const auto& name : supportedExtensionNames) {
		std::cout << "\t" << name << std::endl;
	}
	// Check for and print any unsupported extension
	for (const auto& extension_name : glfwExtensions)
	{
		std::string name(extension_name);
		if (supportedExtensionNames.count(name) <= 0)
		{
			std::cout << "unsupported extension required by GLFW: " << name << std::endl;
		}
	}

	// Enable required extensions
	instance_info.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
	instance_info.ppEnabledExtensionNames = glfwExtensions.data();

	if (ENABLE_VALIDATION_LAYERS) {
		instance_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		instance_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		instance_info.enabledLayerCount = 0;
	}

	VkResult result = vkCreateInstance(&instance_info, nullptr, &instance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}

}

bool VulkanContext::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : VALIDATION_LAYERS)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<const char*> VulkanContext::getRequiredExtensions()
{
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++)
	{
		extensions.push_back(glfwExtensions[i]);
	}

	if (ENABLE_VALIDATION_LAYERS)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
	// should I free sth after?
}

void VulkanContext::setupDebugCallback()
{
	if (!ENABLE_VALIDATION_LAYERS) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugCallback;

	if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug callback!");
	}


}


// Pick up a graphics card to use
void VulkanContext::pickPhysicalDevice()
{
	// This object will be implicitly destroyed when the VkInstance is destroyed, so we don't need to add a delete wrapper.
	VkPhysicalDevice physial_device = VK_NULL_HANDLE;
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

	if (device_count == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device))
		{
			physial_device = device;
			break;
		}
	}

	if (physial_device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to find a suitable GPU!");
	}
	else
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physial_device, &properties);
		std::cout << "Current Device: " << properties.deviceName << std::endl;
	}

	this->physical_device = physial_device;
}

bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device)
{
	//VkPhysicalDeviceProperties properties;
	//vkGetPhysicalDeviceProperties(device, &properties);

	//VkPhysicalDeviceFeatures features;
	//vkGetPhysicalDeviceFeatures(device, &features);

	//return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
	//	&& features.geometryShader;

	QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(device, static_cast<VkSurfaceKHR>(window_surface));

	bool extensions_supported = checkDeviceExtensionSupport(device);

	bool swap_chain_adequate = false;
	if (extensions_supported)
	{
		auto swap_chain_support = SwapChainSupportDetails::querySwapChainSupport(device, static_cast<VkSurfaceKHR>(window_surface));
		swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
	}

	return indices.isComplete() && extensions_supported && swap_chain_adequate;
}

bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

	for (const auto& extension : available_extensions)
	{
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}

void VulkanContext::createLogicalDevice()
{
	QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(physical_device, static_cast<VkSurfaceKHR>(window_surface));

	std::vector <VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<int> queue_families = { indices.graphicsFamily, indices.presentFamily };

	float queue_priority = 1.0f;
	for (int family : queue_families)
	{
		// Create a graphics queue
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = indices.graphicsFamily;
		queue_create_info.queueCount = 1;

		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}

	// Specify used device features
	VkPhysicalDeviceFeatures device_features = {}; // Everything is by default VK_FALSE

												   // Create the logical device
	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pQueueCreateInfos = queue_create_infos.data();
	device_create_info.queueCreateInfoCount = static_cast<uint32_t> (queue_create_infos.size());

	device_create_info.pEnabledFeatures = &device_features;

	if (ENABLE_VALIDATION_LAYERS)
	{
		device_create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		device_create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else
	{
		device_create_info.enabledLayerCount = 0;
	}

	device_create_info.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
	device_create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

	auto result = vkCreateDevice(physical_device, &device_create_info, nullptr, &graphics_device);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(graphics_device, indices.graphicsFamily, 0, &graphics_queue);
	vkGetDeviceQueue(graphics_device, indices.presentFamily, 0, &present_queue);
}

void VulkanContext::createSwapChain()
{
	auto support_details = SwapChainSupportDetails::querySwapChainSupport(physical_device, window_surface);

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
	create_info.surface = window_surface;
	create_info.minImageCount = queue_length;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1; // >1 when developing stereoscopic application
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // render directly
	// VK_IMAGE_USAGE_TRANSFER_DST_BIT and memory operation to enable post processing

	QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(physical_device, window_surface);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily)
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
	create_info.oldSwapchain = old_swap_chain; // required when recreating a swap chain (like resizing windows)

	auto result = vkCreateSwapchainKHR(graphics_device, &create_info, nullptr, &swap_chain);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swap chain!");
	}

	uint32_t image_count;
	vkGetSwapchainImagesKHR(graphics_device, swap_chain, &image_count, nullptr);
	swap_chain_images.resize(image_count);
	vkGetSwapchainImagesKHR(graphics_device, swap_chain, &image_count, swap_chain_images.data());

	swap_chain_image_format = surface_format.format;
	swap_chain_extent = extent;
}

void VulkanContext::createSwapChainImageViews()
{
	swap_chain_imageviews.clear(); // VDeleter will delete old objects
	swap_chain_imageviews.reserve(swap_chain_images.size());

	for (uint32_t i = 0; i < swap_chain_images.size(); i++)
	{
		swap_chain_imageviews.emplace_back( graphics_device, vkDestroyImageView);
		createImageView(swap_chain_images[i], swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, &swap_chain_imageviews[i]);
	}
}

void VulkanContext::createRenderPass()
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

	if (vkCreateRenderPass(graphics_device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void VulkanContext::createDescriptorSetLayout()
{
	// create descriptor for uniform buffer objects
	VkDescriptorSetLayoutBinding ubo_layout_binding = {};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // only referencing from vertex shader
	// VK_SHADER_STAGE_ALL_GRAPHICS
	ubo_layout_binding.pImmutableSamplers = nullptr; // Optional

	// descriptor for texture sampler
	VkDescriptorSetLayoutBinding sampler_layout_binding = {};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.pImmutableSamplers = nullptr;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding lights_layout_binding = {};
	lights_layout_binding.binding = 2;
	lights_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	lights_layout_binding.descriptorCount = 1;
	lights_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // only referencing from vertex shader
																// VK_SHADER_STAGE_ALL_GRAPHICS
	lights_layout_binding.pImmutableSamplers = nullptr; // Optional

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { ubo_layout_binding, sampler_layout_binding, lights_layout_binding };
	// std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding};
	VkDescriptorSetLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = (uint32_t)bindings.size();
	layout_info.pBindings = bindings.data();


	if (vkCreateDescriptorSetLayout(graphics_device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout!");
	}

}

void VulkanContext::createGraphicsPipeline()
{
	auto vert_shader_code = util::readFile(util::getContentPath("helloworld_vert.spv"));
	auto frag_shader_code = util::readFile(util::getContentPath("helloworld_frag.spv"));

	VDeleter<VkShaderModule> vert_shader_module{ graphics_device, vkDestroyShaderModule };
	VDeleter<VkShaderModule> frag_shader_module{ graphics_device, vkDestroyShaderModule };
	createShaderModule(vert_shader_code, &vert_shader_module);
	createShaderModule(frag_shader_code, &frag_shader_module);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
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

	// no uniform variables or push constants
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	VkDescriptorSetLayout set_layouts[] = { descriptor_set_layout };
	pipeline_layout_info.setLayoutCount = 1; // Optional
	pipeline_layout_info.pSetLayouts = set_layouts; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0; // Optional
	pipeline_layout_info.pPushConstantRanges = 0; // Optional


	auto pipeline_layout_result = vkCreatePipelineLayout(graphics_device, &pipeline_layout_info, nullptr,
		&pipeline_layout);
	if (pipeline_layout_result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

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
	pipelineInfo.layout = pipeline_layout;
	pipelineInfo.renderPass = render_pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // not deriving from existing pipeline
	pipelineInfo.basePipelineIndex = -1; // Optional

	auto pipeline_result = vkCreateGraphicsPipelines(graphics_device, VK_NULL_HANDLE, 1
		, &pipelineInfo, nullptr, &graphics_pipeline);

	if (pipeline_result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

void VulkanContext::createFrameBuffers()
{
	swap_chain_framebuffers.clear(); // VDeleter will delete old objects
	swap_chain_framebuffers.reserve(swap_chain_imageviews.size());

	for (size_t i = 0; i < swap_chain_imageviews.size(); i++)
	{
		//swap_chain_framebuffers.push_back(VDeleter<VkFramebuffer>{ graphics_device, vkDestroyFramebuffer });
		swap_chain_framebuffers.emplace_back(graphics_device, vkDestroyFramebuffer);
		std::array<VkImageView, 2> attachments = { swap_chain_imageviews[i], depth_image_view};

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass;
		framebuffer_info.attachmentCount = (uint32_t)attachments.size();
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = swap_chain_extent.width;
		framebuffer_info.height = swap_chain_extent.height;
		framebuffer_info.layers = 1;

		auto result = vkCreateFramebuffer(graphics_device, &framebuffer_info, nullptr, &swap_chain_framebuffers.back());
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}

void VulkanContext::createCommandPool()
{
	auto indices = QueueFamilyIndices::findQueueFamilies(physical_device, window_surface);

	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = indices.graphicsFamily;
	pool_info.flags = 0; // Optional
	// hint the command pool will rerecord buffers by VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
	// allow buffers to be rerecorded individually by VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT

	auto result = vkCreateCommandPool(graphics_device, &pool_info, nullptr, &command_pool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}


void VulkanContext::createDepthResources()
{
	VkFormat depth_format = findDepthFormat();
	createImage(swap_chain_extent.width, swap_chain_extent.height
		, depth_format
		, VK_IMAGE_TILING_OPTIMAL
		, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		, &depth_image
		, &depth_image_memory);
	createImageView(depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, &depth_image_view);
	transitImageLayout(depth_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void VulkanContext::createTextureImage()
{
	// load image file
	int tex_width, tex_height, tex_channels;
	//stbi_uc * pixels = stbi_load("content/jumpin_windy.png"
	//	, &tex_width, &tex_height
	//	, &tex_channels
	//	, STBI_rgb_alpha);
	stbi_uc * pixels = stbi_load(util::TEXTURE_PATH.c_str()
		, &tex_width, &tex_height
		, &tex_channels
		, STBI_rgb_alpha);
	VkDeviceSize image_size = tex_width * tex_height * 4;
	if (!pixels)
	{
		throw std::runtime_error("Failed to load texture image");
	}

	// create staging image memory
	VDeleter<VkImage> staging_image{ graphics_device, vkDestroyImage };
	VDeleter<VkDeviceMemory> staging_image_memory{ graphics_device, vkFreeMemory };
	createImage(tex_width, tex_height
		, VK_FORMAT_R8G8B8A8_UNORM
		, VK_IMAGE_TILING_LINEAR
		, VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		, &staging_image, &staging_image_memory);

	// copy image to staging memory
	void* data;
	vkMapMemory(graphics_device, staging_image_memory, 0, image_size, 0, &data);
	memcpy(data, pixels, (size_t)image_size);
	vkUnmapMemory(graphics_device, staging_image_memory);

	// free image in memory
	stbi_image_free(pixels);

	// create texture image
	createImage(tex_width, tex_height
		, VK_FORMAT_R8G8B8A8_UNORM
		, VK_IMAGE_TILING_OPTIMAL
		, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		, &texture_image
		, &texture_image_memory
		);

	// TODO: doing the steps asynchronously by using a single command buffer
	auto command_buffer = beginSingleTimeCommands();

	recordTransitImageLayout(command_buffer, staging_image, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	recordTransitImageLayout(command_buffer, texture_image, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	recordCopyImage(command_buffer, staging_image, texture_image, tex_width, tex_height);
	recordTransitImageLayout(command_buffer, texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	endSingleTimeCommands(command_buffer);
}

void VulkanContext::createTextureImageView()
{
	createImageView(texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &texture_image_view);
}

void VulkanContext::createTextureSampler()
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


	if (vkCreateSampler(graphics_device, &sampler_info, nullptr, &texture_sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler!");
	}
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

void VulkanContext::createVertexBuffer()
{
	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

	// create staging buffer
	VDeleter<VkBuffer> staging_buffer{ graphics_device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> staging_buffer_memory{ graphics_device, vkFreeMemory };
	createBuffer(buffer_size
		, VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		, &staging_buffer
		, &staging_buffer_memory);

	// copy data to staging buffer
	void* data;
	vkMapMemory(graphics_device, staging_buffer_memory, 0, buffer_size, 0, &data); // access the graphics memory using mapping
		memcpy(data, vertices.data(), (size_t)buffer_size); // may not be immediate due to memory caching or write operation not visiable without VK_MEMORY_PROPERTY_HOST_COHERENT_BIT or explict flusing
	vkUnmapMemory(graphics_device, staging_buffer_memory);

	// create vertex buffer at optimized local memory which may not be directly accessable by memory mapping
	// as copy destination of staging buffer
	createBuffer(buffer_size
		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		, &vertex_buffer
		, &vertex_buffer_memory);

	// copy content of staging buffer to vertex buffer
	copyBuffer(staging_buffer, vertex_buffer, buffer_size);
}

void VulkanContext::createIndexBuffer()
{
	VkDeviceSize buffer_size = sizeof(vertex_indices[0]) * vertex_indices.size();

	// create staging buffer
	VDeleter<VkBuffer> staging_buffer{ graphics_device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> staging_buffer_memory{ graphics_device, vkFreeMemory };
	createBuffer(buffer_size
		, VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		, &staging_buffer
		, &staging_buffer_memory);

	void* data;
	vkMapMemory(graphics_device, staging_buffer_memory, 0, buffer_size, 0, &data); // access the graphics memory using mapping
	memcpy(data, vertex_indices.data(), (size_t)buffer_size); // may not be immediate due to memory caching or write operation not visiable without VK_MEMORY_PROPERTY_HOST_COHERENT_BIT or explict flusing
	vkUnmapMemory(graphics_device, staging_buffer_memory);

	createBuffer(buffer_size
		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		, &index_buffer
		, &index_buffer_memory);

	// copy content of staging buffer to index buffer
	copyBuffer(staging_buffer, index_buffer, buffer_size);
}

void VulkanContext::createUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	createBuffer(bufferSize
		, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		, &uniform_staging_buffer
		, &uniform_staging_buffer_memory);
	createBuffer(bufferSize
		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
		, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		, &uniform_buffer
		, &uniform_buffer_memory);
}

void VulkanContext::createLights()
{
	pointlights.emplace_back(glm::vec3(0.f, 1.f, 0.f), 10, glm::vec3(1.0f, 0.0f, 0.0f));  // TODO: test only
	pointlights.emplace_back(glm::vec3(2.f, 1.f, 1.f), 10, glm::vec3(1.0f, 1.0f, 0.0f));
	pointlights.emplace_back(glm::vec3(0.f, 1.f, 0.f), 10, glm::vec3(1.0f, 0.0f, 1.0f));

	// TODO: choose between memory mapping and staging buffer
	//  (given that the lights are moving)


	auto size = sizeof(PointLight) * pointlights.size();

	VkDeviceSize bufferSize = size;

	//createBuffer(bufferSize
	//	, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
	//	, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	//	, &pointlight_buffer
	//	, &pointlight_buffer_memory);

	createBuffer(bufferSize
		, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT   // TODO: uniform or storage?
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		, &pointlight_buffer
		, &pointlight_buffer_memory);

	void* data;

	vkMapMemory(graphics_device, pointlight_buffer_memory, 0, bufferSize, 0, &data);
	memcpy(data, pointlights.data(), size);
	vkUnmapMemory(graphics_device, pointlight_buffer_memory);

}

void VulkanContext::createDescriptorPool()
{
	// Create descriptor pool for uniform buffer
	std::array<VkDescriptorPoolSize, 3> pool_sizes = {};
	//std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = 1;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = 1;
	pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = (uint32_t)pool_sizes.size();
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = 1;
	pool_info.flags = 0;
	//poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(graphics_device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool!");
	}
}

void VulkanContext::createDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptor_set_layout };
	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = descriptor_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(graphics_device, &alloc_info, &descriptor_set) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor set!");
	}

	// refer to the uniform object buffer
	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = uniform_buffer;
	buffer_info.offset = 0;
	buffer_info.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo image_info = {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView = texture_image_view;
	image_info.sampler = texture_sampler;

	VkDescriptorBufferInfo lights_buffer_info = {};
	lights_buffer_info.buffer = pointlight_buffer;
	lights_buffer_info.offset = 0;
	lights_buffer_info.range = sizeof(PointLight) * pointlights.size();

	std::array<VkWriteDescriptorSet, 3> descriptor_writes = {};
	//std::array<VkWriteDescriptorSet, 2> descriptor_writes = {};

	// ubo
	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &buffer_info;
	descriptor_writes[0].pImageInfo = nullptr; // Optional
	descriptor_writes[0].pTexelBufferView = nullptr; // Optional

	// texture
	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pImageInfo = &image_info;

	// lights
	descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[2].dstSet = descriptor_set;
	descriptor_writes[2].dstBinding = 2;
	descriptor_writes[2].dstArrayElement = 0;
	descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[2].descriptorCount = 1;
	descriptor_writes[2].pBufferInfo = &lights_buffer_info;
	descriptor_writes[2].pImageInfo = nullptr; // Optional
	descriptor_writes[2].pTexelBufferView = nullptr; // Optional


	vkUpdateDescriptorSets(graphics_device, (uint32_t)descriptor_writes.size()
		, descriptor_writes.data(), 0, nullptr);
}

void VulkanContext::createCommandBuffers()
{
	// Free old command buffers, if any
	if (command_buffers.size() > 0)
	{
		vkFreeCommandBuffers(graphics_device, command_pool, (uint32_t)command_buffers.size(), command_buffers.data());
	}

	command_buffers.resize(swap_chain_framebuffers.size());

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool;
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

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = render_pass;
		render_pass_info.framebuffer = swap_chain_framebuffers[i];
		render_pass_info.renderArea.offset = { 0, 0 };
		render_pass_info.renderArea.extent = swap_chain_extent;

		std::array<VkClearValue, 2> clear_values = {};
		clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clear_values[1].depthStencil = { 1.0f, 0 }; // 1.0 is far view plane
		render_pass_info.clearValueCount = (uint32_t)clear_values.size();
		render_pass_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

		// bind vertex buffer
		VkBuffer vertex_buffers[] = { vertex_buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);
		//vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS
			, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
		// TODO: better to store vertex buffer and index buffer in a single VkBuffer

		//vkCmdDraw(command_buffers[i], VERTICES.size(), 1, 0, 0);
		vkCmdDrawIndexed(command_buffers[i], (uint32_t)vertex_indices.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(command_buffers[i]);

		auto record_result = vkEndCommandBuffer(command_buffers[i]);
		if (record_result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}
}

void VulkanContext::createSemaphores()
{
	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	auto result1 = vkCreateSemaphore(graphics_device, &semaphore_info, nullptr, &image_available_semaphore);
	auto result2 = vkCreateSemaphore(graphics_device, &semaphore_info, nullptr, &render_finished_semaphore);

	if (result1 != VK_SUCCESS || result2 != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create semaphores!");
	}
}

void VulkanContext::updateUniformBuffer()
{
	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0f;
	UniformBufferObject ubo = {};
	
	ubo.model = glm::mat4(1.0f);
	ubo.view = view_matrix;
	//ubo.model = glm::rotate(glm::mat4(), time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.view = glm::lookAt(glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swap_chain_extent.width / (float)swap_chain_extent.height, 0.5f, 100.0f);
	ubo.proj[1][1] *= -1; //since the Y axis of Vulkan NDC points down

	void* data;
	vkMapMemory(graphics_device, uniform_staging_buffer_memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(graphics_device, uniform_staging_buffer_memory);

	copyBuffer(uniform_staging_buffer, uniform_buffer, sizeof(ubo));
	// TODO: maybe I shouldn't use single time buffer

	//TODO: use push constants
}

const uint64_t ACQUIRE_NEXT_IMAGE_TIMEOUT{ std::numeric_limits<uint64_t>::max() };
void VulkanContext::drawFrame()
{
	// 1. Acquiring an image from the swap chain
	uint32_t image_index;
	auto aquiring_result = vkAcquireNextImageKHR(graphics_device, swap_chain
		, ACQUIRE_NEXT_IMAGE_TIMEOUT, image_available_semaphore, VK_NULL_HANDLE, &image_index);

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

	// 2. Submitting the command buffer
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore wait_semaphores[] = { image_available_semaphore }; // which semaphore to wait
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // which stage to execute
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffers[image_index];
	VkSemaphore signal_semaphores[] = { render_finished_semaphore };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	auto submit_result = vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	if (submit_result != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	// 3. Submitting the result back to the swap chain to show it on screen
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;
	VkSwapchainKHR swapChains[] = { swap_chain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapChains;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr; // Optional, check for if every single chains is successful

	auto present_result = vkQueuePresentKHR(present_queue, &present_info);

	if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
	{
		recreateSwapChain();
	}
	else if (present_result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image!");
	}
}

void VulkanContext::createShaderModule(const std::vector<char>& code, VkShaderModule* p_shader_module)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = (uint32_t*)code.data();

	auto result = vkCreateShaderModule(graphics_device, &create_info, nullptr, p_shader_module);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}
}

VkSurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
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

VkPresentModeKHR VulkanContext::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
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

VkExtent2D VulkanContext::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
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

VkFormat VulkanContext::findSupportedFormat(const std::vector<VkFormat>& candidates
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

void VulkanContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags property_bits
	, VkBuffer* p_buffer, VkDeviceMemory* p_buffer_memory)
{
	// create vertex buffer
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // vertex buffer only used in graphics queue
	buffer_info.flags = 0;

	auto buffer_result = vkCreateBuffer(graphics_device, &buffer_info, nullptr, p_buffer);

	if (buffer_result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create buffer!");
	}

	// allocate memory for buffer
	VkMemoryRequirements memory_req;
	vkGetBufferMemoryRequirements(graphics_device, *p_buffer, &memory_req);

	VkMemoryAllocateInfo memory_alloc_info = {};
	memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_alloc_info.allocationSize = memory_req.size;
	memory_alloc_info.memoryTypeIndex = findMemoryType(memory_req.memoryTypeBits
		, property_bits
		, physical_device);

	auto memory_result = vkAllocateMemory(graphics_device, &memory_alloc_info, nullptr, p_buffer_memory);
	if (memory_result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	// bind buffer with memory
	auto bind_result = vkBindBufferMemory(graphics_device, *p_buffer, *p_buffer_memory, 0);
	if (bind_result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to bind buffer memory!");
	}
}

void VulkanContext::copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
	VkCommandBuffer copy_command_buffer = beginSingleTimeCommands();

	recordCopyBuffer(copy_command_buffer, src_buffer, dst_buffer, size);

	endSingleTimeCommands(copy_command_buffer);
}

void VulkanContext::createImage(uint32_t image_width, uint32_t image_height
	, VkFormat format, VkImageTiling tiling
	, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties
	, VkImage* p_vkimage, VkDeviceMemory* p_image_memory)
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

	if (vkCreateImage(graphics_device, &image_info, nullptr, p_vkimage) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}
	VkImage vkimage = *p_vkimage;

	// allocate image memory
	VkMemoryRequirements memory_req;
	vkGetImageMemoryRequirements(graphics_device, vkimage, &memory_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_req.size;
	alloc_info.memoryTypeIndex = findMemoryType(memory_req.memoryTypeBits
		, memory_properties
		, physical_device);

	if (vkAllocateMemory(graphics_device, &alloc_info, nullptr, p_image_memory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(graphics_device, vkimage, *p_image_memory, 0);
}

void VulkanContext::copyImage(VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height)
{
	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	recordCopyImage(command_buffer, src_image, dst_image, width, height);

	endSingleTimeCommands(command_buffer);
}

void VulkanContext::transitImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	recordTransitImageLayout(command_buffer, image, old_layout, new_layout);

	endSingleTimeCommands(command_buffer);
}

void VulkanContext::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_mask, VkImageView* p_image_view)
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

// create a temperorary command buffer for one-time use
// and begin recording
VkCommandBuffer VulkanContext::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
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
void VulkanContext::endSingleTimeCommands(VkCommandBuffer command_buffer)
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
	vkFreeCommandBuffers(graphics_device, command_pool, 1, &command_buffer);
}

void VulkanContext::recordCopyBuffer(VkCommandBuffer command_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
	VkBufferCopy copy_region = {};
	copy_region.srcOffset = 0; // Optional
	copy_region.dstOffset = 0; // Optional
	copy_region.size = size;

	vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
}

void VulkanContext::recordCopyImage(VkCommandBuffer command_buffer, VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height)
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

void VulkanContext::recordTransitImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
	// barrier is used to ensure a buffer has finished writing before
	// reading as weel as doing transition
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
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
