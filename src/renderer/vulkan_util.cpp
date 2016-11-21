// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "vulkan_util.h"

#include "../util.h"

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
