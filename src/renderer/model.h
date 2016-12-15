// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include "raii.h"

#include <vulkan/vulkan.hpp>

#include <vector>

class VContext;

/**
* A structure that points to a part of a buffer
*/
struct VBufferSection
{
	vk::Buffer buffer = {};  // just a handle, no ownership for this buffer
	vk::DeviceSize offset = 0;
	vk::DeviceSize size = 0;

	VBufferSection() = default;

	VBufferSection(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size)
		: buffer(buffer)
		, offset(offset)
		, size(size)
	{}
};

struct VMeshPart
{
	// todo: separate mesh part with material?
	// (material as another global storage??)
	VBufferSection vertex_buffer_section = {};
	VBufferSection index_buffer_section = {};
	VBufferSection material_uniform_buffer_section = {};
	size_t index_count = 0;
	vk::DescriptorSet material_descriptor_set = {};  // TODO: I still need a per-instance descriptor set


	// handles for images (no ownership or so)
	vk::ImageView albedo_map = {};
	vk::ImageView normal_map = {};



	VMeshPart(const VBufferSection& vertex_buffer_section, const VBufferSection& index_buffer_section, size_t index_count)
		: vertex_buffer_section(vertex_buffer_section)
		, index_buffer_section(index_buffer_section)
		, index_count(index_count)
	{}
};

/**
* A class to manage resources of a static mesh.
* Must be destructed before the vk::Device used to construct it
*/
class VModel
{
public:
	VModel() = default;
	~VModel() = default;
	VModel(VModel&&) = default;
	VModel& operator= (VModel&&) = default;

	const std::vector<VMeshPart>& getMeshParts() const
	{
		return mesh_parts;
	}

	static VModel loadModelFromFile(const VContext& vulkan_context, const std::string& path
		, const vk::Sampler& texture_sampler, const vk::DescriptorPool& descriptor_pool,
		const vk::DescriptorSetLayout& material_descriptor_set_layout);

	VModel(const VModel&) = delete;
	VModel& operator= (const VModel&) = delete;

private:
	VRaii<VkBuffer> buffer;
	VRaii<VkDeviceMemory> buffer_memory;
	std::vector<VRaii<VkImage>> images;
	std::vector<VRaii<VkImageView>> imageviews;
	std::vector<VRaii<VkDeviceMemory>> image_memories; //TODO: use a single memory, or two
	VRaii<VkBuffer> uniform_buffer;
	VRaii<VkDeviceMemory> uniform_buffer_memory;

	std::vector<VMeshPart> mesh_parts;

};

