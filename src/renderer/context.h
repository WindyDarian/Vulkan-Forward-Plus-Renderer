// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include "raii.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <vector>

struct GLFWwindow;

struct QueueFamilyIndices
{
	int graphics_family = -1;
	int present_family = -1;
	int compute_family = -1;

	bool isComplete()
	{
		return graphics_family >= 0 && present_family >= 0 && compute_family >= 0;
	}

	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;

	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
};

/**
* the vulkan context we needed like instance, physical device and logical device
*/
class VContext
{
public:
	VContext(GLFWwindow* window);
	~VContext();

	VContext(VContext&&) = delete;
	VContext& operator= (VContext&&) = delete;
	VContext(const VContext&) = delete;
	VContext& operator= (const VContext&) = delete;

	std::pair<int, int> getWindowFrameBufferSize();

private:

	GLFWwindow* window;

	VDeleter<VkInstance> instance{ vkDestroyInstance };
	VDeleter<VkDebugReportCallbackEXT> callback{ instance, DestroyDebugReportCallbackEXT };


public: // TODO: make them private after I removed all VDeleter
	QueueFamilyIndices queue_family_indices;
	VkPhysicalDevice physical_device;
	VDeleter<VkDevice> graphics_device{ vkDestroyDevice }; //logical device
	vk::Device device; // vulkan.hpp wraper for graphics_device, maybe I should migrate all code to vulkan-hpp
	vk::Queue graphics_queue;
	VDeleter<VkSurfaceKHR> window_surface{ instance, vkDestroySurfaceKHR };
	VkQueue present_queue;
	vk::Queue compute_queue;
private:

	static void DestroyDebugReportCallbackEXT(VkInstance instance
		, VkDebugReportCallbackEXT callback
		, const VkAllocationCallbacks* pAllocator);

	static VkResult CreateDebugReportCallbackEXT(VkInstance instance
		, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo
		, const VkAllocationCallbacks* pAllocator
		, VkDebugReportCallbackEXT* pCallback);

	void createInstance();
	void setupDebugCallback();
	void createWindowSurface();
	void pickPhysicalDevice();
	void findQueueFamilyIndices();
	void createLogicalDevice();

	void initVulkan()
	{
		createInstance();
		setupDebugCallback();
		createWindowSurface();
		pickPhysicalDevice();
		findQueueFamilyIndices();
		createLogicalDevice();
	}
};

