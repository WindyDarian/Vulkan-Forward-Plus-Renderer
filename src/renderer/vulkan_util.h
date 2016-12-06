// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.
#pragma once

#include <vulkan/vulkan.h>
#include <array>

#include <string>

namespace vulkan_util
{
	VkVertexInputBindingDescription getVertexBindingDesciption();

	std::array<VkVertexInputAttributeDescription, 4> getVertexAttributeDescriptions();

	void checkResult(VkResult result, const char * what = "Runtime error from vulkan_util::checkResult!");

}

