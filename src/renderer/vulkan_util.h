// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.
#pragma once

#include "raii.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <array>
#include <string>

namespace vulkan_util
{
	VkVertexInputBindingDescription getVertexBindingDesciption();

	std::array<VkVertexInputAttributeDescription, 4> getVertexAttributeDescriptions();

	void checkResult(VkResult result, const char * what = "Runtime error from vulkan_util::checkResult!");

}

class VContext;

/**
* a utility module for vulkan context
*/
class VUtility
{
public:

	VUtility(const VContext& context);
	~VUtility() = default;

	VUtility(VUtility&&) = delete;
	VUtility& operator= (VUtility&&) = delete;
	VUtility(const VUtility&) = delete;
	VUtility& operator= (const VUtility&) = delete;

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
	void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size, VkDeviceSize src_offset = 0, VkDeviceSize dst_offset = 0);

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
	void recordCopyBuffer(VkCommandBuffer command_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size, VkDeviceSize src_offset = 0, VkDeviceSize dst_offset = 0);
	void recordCopyImage(VkCommandBuffer command_buffer, VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height);
	void recordTransitImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);

private:

	std::tuple<VkBuffer, VkDeviceMemory> createBufferImpl(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags property_bits, int sharing_queue_family_index_a = -1, int sharing_queue_family_index_b = -1);


	const VContext* context;
	vk::PhysicalDevice physical_device;
	VkDevice graphics_device; // for previously written code using C api
	vk::Device device;
	vk::Queue graphics_queue;
	vk::Queue compute_queue;
	vk::Queue present_queue;
	vk::CommandPool graphics_queue_command_pool;
	vk::CommandPool compute_queue_command_pool;



};
