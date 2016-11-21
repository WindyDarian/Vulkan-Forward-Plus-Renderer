// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.
#pragma once

#include <vulkan/vulkan.h>
#include <array>

namespace vulkan_util
{
	VkVertexInputBindingDescription getVertexBindingDesciption();

	std::array<VkVertexInputAttributeDescription, 3> getVertexAttributeDescriptions();
}

