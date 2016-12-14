// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include "raii.h"

#include <vulkan/vulkan.hpp>

#include <vector>

/**
* A structure that points to a part of a buffer
*/
struct VBufferSection
{
	vk::Buffer buffer;  // just a handle, no ownership for this buffer
	vk::DeviceSize offset;
	vk::DeviceSize size;

	VBufferSection(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size)
		: buffer(buffer)
		, offset(offset)
		, size(size)
	{}
};

struct VMeshPart
{
	VBufferSection vertex_buffer_section;
	VBufferSection index_buffer_section;

	// handles for images (no ownership or so)
	vk::Image albedo_map;
	vk::Image normal_map;
};

/**
* A class to manage resources of a static mesh.
* Must be destructed before the vk::Device used to construct it
*/
class VModel
{
public:
	~VModel() = default;

	VModel(VModel&&) = default;
	VModel& operator= (VModel&&) = default;

	VModel(const VModel&) = delete;
	VModel& operator= (const VModel&) = delete;

private:
	VModel() = default;

	VRaii<vk::Buffer> buffer;
	//std::vector<VRaii<vk::Image>> images;
	std::vector<VMeshPart> mesh_parts;

	const std::vector<VMeshPart>& getMeshParts() const
	{
		return mesh_parts;
	}

	static VModel loadModelFromFile(vk::Device device_handle, const std::string& path);
};

