// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "vulkan_util.h"

#include "context.h"
#include "../util.h"

#include <stb_image.h>

VkVertexInputBindingDescription vulkan_util::getVertexBindingDesciption()
{
	using util::Vertex;
	VkVertexInputBindingDescription binding_description = {};
	binding_description.binding = 0; // index of the binding, defined in vertex shader
	binding_description.stride = sizeof(Vertex);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // move to next data engty after each vertex
	return binding_description;
}

std::array<VkVertexInputAttributeDescription, 4> vulkan_util::getVertexAttributeDescriptions()
{
	using util::Vertex;
	std::array<VkVertexInputAttributeDescription, 4> attr_descriptions = {};
	attr_descriptions[0].binding = 0;
	attr_descriptions[0].location = 0;
	attr_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attr_descriptions[0].offset = offsetof(Vertex, pos); //bytes of a member since beginning of struct
	attr_descriptions[1].binding = 0;
	attr_descriptions[1].location = 1;
	attr_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attr_descriptions[1].offset = offsetof(Vertex, color); //bytes of a member since beginning of struct
	attr_descriptions[2].binding = 0;
	attr_descriptions[2].location = 2;
	attr_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attr_descriptions[2].offset = offsetof(Vertex, tex_coord);
	// normal
	attr_descriptions[3].binding = 0;
	attr_descriptions[3].location = 3;
	attr_descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attr_descriptions[3].offset = offsetof(Vertex, normal);

	return attr_descriptions;
}

void vulkan_util::checkResult(VkResult result, const char * what)
{

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(what);
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

VUtility::VUtility(const VContext & context)
	: context(&context)
	, physical_device(context.getPhysicalDevice())
	, graphics_device(context.getDevice())
	, device(graphics_device)
	, graphics_queue(context.getGraphicsQueue())
	, compute_queue(context.getComputeQueue())
	, present_queue(context.getPresentQueue())
	, graphics_queue_command_pool(context.getGraphicsCommandPool())
	, compute_queue_command_pool(context.getComputeCommandPool())
{}

VkSurfaceFormatKHR VUtility::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
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

VkPresentModeKHR VUtility::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
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

VkExtent2D VUtility::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	// The swap extent is the resolution of the swap chain images
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int window_framebuffer_width, window_framebuffer_height;
		std::tie(window_framebuffer_width, window_framebuffer_height) = context->getWindowFrameBufferSize();
		VkExtent2D actual_extent = { (uint32_t)window_framebuffer_width, (uint32_t)window_framebuffer_height };

		actual_extent.width = std::max(capabilities.minImageExtent.width
			, std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height
			, std::min(capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}

VkFormat VUtility::findSupportedFormat(const std::vector<VkFormat>& candidates
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

std::tuple<VRaii<VkBuffer>, VRaii<VkDeviceMemory>> VUtility::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags property_bits
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
		buffer_info.queueFamilyIndexCount = static_cast<uint32_t>(indices.size());
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

void VUtility::copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size, VkDeviceSize src_offset, VkDeviceSize dst_offset)
{
	VkCommandBuffer copy_command_buffer = beginSingleTimeCommands();

	recordCopyBuffer(copy_command_buffer, src_buffer, dst_buffer, size, src_offset, dst_offset);

	endSingleTimeCommands(copy_command_buffer);
}

std::tuple<VRaii<VkImage>, VRaii<VkDeviceMemory>> VUtility::createImage(uint32_t image_width, uint32_t image_height
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

void VUtility::copyImage(VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height)
{
	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	recordCopyImage(command_buffer, src_image, dst_image, width, height);

	endSingleTimeCommands(command_buffer);
}

void VUtility::transitImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkCommandBuffer command_buffer = beginSingleTimeCommands();

	recordTransitImageLayout(command_buffer, image, old_layout, new_layout);

	endSingleTimeCommands(command_buffer);

}

void VUtility::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_mask, VkImageView* p_image_view)
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


VRaii<VkImageView> VUtility::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_mask)
{
	VkImageView img_view;
	createImageView(image, format, aspect_mask, &img_view);
	return VRaii<VkImageView>(img_view, [device = this->device](auto& obj) {device.destroyImageView(obj); });
}

std::tuple<VRaii<VkImage>, VRaii<VkDeviceMemory>, VRaii<VkImageView>> VUtility::loadImageFromFile(std::string path)
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
VkCommandBuffer VUtility::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = graphics_queue_command_pool;
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
void VUtility::endSingleTimeCommands(VkCommandBuffer command_buffer)
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
	vkFreeCommandBuffers(graphics_device, graphics_queue_command_pool, 1, &command_buffer);
}

void VUtility::recordCopyBuffer(VkCommandBuffer command_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size, VkDeviceSize src_offset, VkDeviceSize dst_offset)
{
	VkBufferCopy copy_region = {};
	copy_region.srcOffset = src_offset; // Optional
	copy_region.dstOffset = dst_offset; // Optional
	copy_region.size = size;

	vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
}

void VUtility::recordCopyImage(VkCommandBuffer command_buffer, VkImage src_image, VkImage dst_image, uint32_t width, uint32_t height)
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

void VUtility::recordTransitImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
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
