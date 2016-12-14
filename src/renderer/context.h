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

	auto getQueueFamilyIndices()
	{
		return queue_family_indices;
	}

	vk::PhysicalDevice getPhysicalDevice()
	{
		return physical_device;
	}

	vk::Device getDevice()
	{
		return graphics_device.get();
	}

	vk::Queue getGraphicsQueue()
	{
		return graphics_queue;
	}

	vk::Queue getPresentQueue()
	{
		return present_queue;
	}

	vk::Queue getComputeQueue()
	{
		return compute_queue;
	}

	vk::SurfaceKHR getWindowSurface()
	{
		return window_surface.get();
	}

private:

	GLFWwindow* window;

	VRaii<vk::Instance> instance;
	VRaii<vk::DebugReportCallbackEXT> callback;
	VRaii<vk::Device> graphics_device; //logical device
	VRaii<vk::SurfaceKHR> window_surface;
	//VDeleter<VkSurfaceKHR> window_surface{ instance, vkDestroySurfaceKHR };

	QueueFamilyIndices queue_family_indices;
	VkPhysicalDevice physical_device;
	vk::Queue graphics_queue;
	vk::Queue present_queue;
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

