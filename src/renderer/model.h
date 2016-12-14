// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include "raii.h"

#include <vulkan/vulkan.hpp>


/**
* A class to manage resources of a model
*/
class VModel
{
	VRaii<vk::Buffer> buffer;
	VRaii<vk::Image> image;
};
