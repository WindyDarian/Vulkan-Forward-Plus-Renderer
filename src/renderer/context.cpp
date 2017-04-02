// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "context.h"

#include <GLFW/glfw3.h>

#include <unordered_set>
#include <iostream>
#include <cstring>
#include <set>

#ifdef NDEBUG
// if not debugging
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

// #define ONE_QUEUE // Force one queue for everything

const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VContext::VContext(GLFWwindow* window)
{
	if (!window)
	{
		throw std::runtime_error("invalid window");
	}

	this->window = window;

	initVulkan();
}

VContext::~VContext() = default;

std::pair<int, int> VContext::getWindowFrameBufferSize() const
{
	int framebuffer_width, framebuffer_height;
	glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
	return std::make_pair(framebuffer_width, framebuffer_height);
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
#ifdef ONE_QUEUE //TODO: refactor...
		if (queuefamily.queueCount > 0 && (queuefamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queuefamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (queuefamily.queueCount > 0 && presentSupport)
			{
				indices.graphics_family = i;
				indices.present_family = i;
			}
		}
#else
		if (queuefamily.queueCount > 0 && (queuefamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			auto support_compute = static_cast<bool>(queuefamily.queueFlags & VK_QUEUE_COMPUTE_BIT);
			auto enough_size = (queuefamily.queueCount >= 2);

			if (!support_compute)
			{
				std::cout << "Found a graphics queue family, but it doesn't support compute." << std::endl;
			}
			if (!enough_size) // using a second queue to do light culling
			{
				std::cout << "Found a graphics queue family, but it doesn't enough queue count" << std::endl;
			}

			if (support_compute && enough_size)
			{
				indices.graphics_family = i;
			}
		}

		//if (queuefamily.queueCount > 0 && queuefamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
		//{
		//	indices.compute_family = i;
		//}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (queuefamily.queueCount > 0 && presentSupport)
		{
			indices.present_family = i;
		}
#endif // ONE_QUEUE



		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

std::vector<const char*> getRequiredExtensions()
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

VkResult VContext::CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
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

void VContext::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

bool checkValidationLayerSupport()
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

void VContext::createInstance()
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

	instance = VRaii<vk::Instance>{
		vk::createInstance(instance_info, nullptr),
		[](vk::Instance& obj)
		{
			obj.destroy();
		}
	};

}

void VContext::setupDebugCallback()
{
	if (!ENABLE_VALIDATION_LAYERS) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugCallback;

	VkDebugReportCallbackEXT temp_callback;
	if (CreateDebugReportCallbackEXT(instance.get(), &createInfo, nullptr, &temp_callback) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug callback!");
	}

	callback = VRaii<vk::DebugReportCallbackEXT>{
		temp_callback,
		[instance = this->instance.get()](auto& obj)
		{
			DestroyDebugReportCallbackEXT(instance, obj, nullptr);
		}
	};

}

bool checkDeviceExtensionSupport(VkPhysicalDevice device)
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


bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR window_surface)
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

// Pick up a graphics card to use
void VContext::pickPhysicalDevice()
{
	// This object will be implicitly destroyed when the VkInstance is destroyed, so we don't need to add a delete wrapper.
	VkPhysicalDevice physial_device = VK_NULL_HANDLE;
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance.get(), &device_count, nullptr);

	if (device_count == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance.get(), &device_count, devices.data());

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device, window_surface.get()))
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


	this->physical_device_properties = static_cast<vk::PhysicalDevice>(physical_device).getProperties();
}

void VContext::findQueueFamilyIndices()
{
	queue_family_indices = QueueFamilyIndices::findQueueFamilies(physical_device, window_surface.get());

	if (!queue_family_indices.isComplete())
	{
		throw std::runtime_error("Queue family indices not complete!");
	}
}

// Needs to be called right after instance creation because it may influence device selection
void VContext::createWindowSurface()
{
	VkSurfaceKHR temp_surface;
	auto result = glfwCreateWindowSurface(instance.get(), window, nullptr, &temp_surface);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface!");
	}

	window_surface = VRaii<vk::SurfaceKHR>{
		temp_surface,
		[instance = this->instance.get()](auto& surface)
		{
			instance.destroySurfaceKHR(surface);
		}
	};
}

void VContext::createLogicalDevice()
{
	QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(physical_device, static_cast<VkSurfaceKHR>(window_surface.get()));

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

	std::vector<int> queue_families;// = { indices.graphics_family, indices.present_family };
	std::vector<std::vector<float>> queue_priorties;// = { { 1.0f, 1.0f },{ 1.0f } }; // 2 queues in graphics family, 1 used for light cullingf

#ifdef ONE_QUEUE
	queue_families = { indices.graphics_family};
	queue_priorties = { { 1.0f } }; // 2 queues in graphics family, 1 used for light cullingf
#else
	if (indices.graphics_family != indices.present_family)
	{
		//TODO: refactor this part of code, use some structs to help queue creatrion
		queue_families = { indices.graphics_family, indices.present_family };
		queue_priorties = { { 1.0f, 1.0f },{ 1.0f } }; // 2 queues in graphics family, 1 used for light cullingf
	}
	else
	{
		queue_families = { indices.graphics_family };
		queue_priorties = { { 1.0f, 1.0f, 1.0f } };
	}
#endif // ONE_QUEUE

	float queue_priority = 1.0f;
	for (int i = 0; i < queue_families.size(); i++)
	{
		auto family = queue_families[i];

		// Create a graphics queue
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = indices.graphics_family;
		queue_create_info.queueCount = static_cast<uint32_t>(queue_priorties[i].size());

		queue_create_info.pQueuePriorities = queue_priorties[i].data();
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

	VkDevice temp_device;
	auto result = vkCreateDevice(physical_device, &device_create_info, nullptr, &temp_device);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device!");
	}
	graphics_device = VRaii<vk::Device>{
		temp_device,
		[](vk::Device& device)
		{
			device.destroy();
		}
	};
	auto device = graphics_device.get();

#ifdef ONE_QUEUE
	graphics_queue = device.getQueue(indices.graphics_family, 0);
	compute_queue = device.getQueue(indices.graphics_family, 0);
	present_queue = device.getQueue(indices.graphics_family, 0);
#else
	graphics_queue = device.getQueue(indices.graphics_family, 0);
	compute_queue = device.getQueue(indices.graphics_family, 1);

	if (indices.graphics_family != indices.present_family)
	{
		// TODO: refactor this part of code, use some structs to help queue creatrion
		present_queue = device.getQueue(indices.present_family, 0);
	}
	else
	{
		present_queue = device.getQueue(indices.graphics_family, 2);
	}
#endif // ONE_QUEUE

}

void VContext::createCommandPools()
{
	auto device = graphics_device.get();
	auto raii_commandpool_deleter = [device = device](auto& obj)
	{
		device.destroyCommandPool(obj);
	};

	auto& indices = queue_family_indices;

	// graphics_queue_command_pool
	{
		VkCommandPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex = indices.graphics_family;
		pool_info.flags = 0; // Optional
								// hint the command pool will rerecord buffers by VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
								// allow buffers to be rerecorded individually by VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT



		graphics_queue_command_pool = VRaii<vk::CommandPool>(
			device.createCommandPool(pool_info, nullptr),
			raii_commandpool_deleter
		);
	}
	
	// compute_queue_command_pool
	{
		VkCommandPoolCreateInfo cmd_pool_info = {};
		cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_pool_info.queueFamilyIndex = indices.graphics_family;
		cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;


		compute_queue_command_pool = VRaii<vk::CommandPool>(
			device.createCommandPool(cmd_pool_info, nullptr),
			raii_commandpool_deleter
		);
	}

}

SwapChainSupportDetails SwapChainSupportDetails::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
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

//// for use later
//std::tuple<VRaii<vk::Image>, VRaii<vk::DeviceMemory>> _VulkanRenderer_Impl::createImage(uint32_t image_width, uint32_t image_height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memory_properties)
//{
//
//	vk::ImageCreateInfo image_info = {};
//	image_info.imageType = vk::ImageType::e2D;
//	image_info.extent.width = image_width;
//	image_info.extent.height = image_height;
//	image_info.extent.depth = 1;
//	image_info.mipLevels = 1;
//	image_info.arrayLayers = 1;
//	image_info.format = format; //VK_FORMAT_R8G8B8A8_UNORM;
//	image_info.tiling = tiling; //VK_IMAGE_TILING_LINEAR;
//								// VK_IMAGE_TILING_OPTIMAL is better if don't need to directly access memory
//	image_info.initialLayout = vk::ImageLayout::ePreinitialized; // VK_IMAGE_LAYOUT_UNDEFINED for attachments like color/depth buffer
//
//	image_info.usage = usage; //VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
//	image_info.sharingMode = vk::SharingMode::eExclusive; // one queue family only
//	image_info.samples = vk::SampleCountFlagBits::e1;
//	image_info.flags = vk::ImageCreateFlags(); // there are flags for sparse image (well not the case)
//
//	vk::Image image = device.createImage(image_info, nullptr);
//
//	// allocate image memory
//	VkMemoryRequirements memory_req;
//	vkGetImageMemoryRequirements(graphics_device, image, &memory_req);
//
//	VkMemoryAllocateInfo alloc_info = {};
//	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//	alloc_info.allocationSize = memory_req.size;
//	alloc_info.memoryTypeIndex = findMemoryType(memory_req.memoryTypeBits
//		, memory_properties
//		, physical_device);
//
//	vk::DeviceMemory image_memory = device.allocateMemory(alloc_info, nullptr);
//
//	device.bindImageMemory(image, image_memory, 0);
//
//	auto raii_image_deleter = [device = this->device](auto& obj)
//	{
//		device.destroyImage(obj);
//	};
//
//	auto raii_memory_deleter = [device = this->device](auto& obj)
//	{
//		device.freeMemory(obj);
//	};
//
//	return std::make_tuple(VRaii<vk::Image>(image, raii_image_deleter), VRaii<vk::DeviceMemory>(image_memory, raii_memory_deleter));
//}