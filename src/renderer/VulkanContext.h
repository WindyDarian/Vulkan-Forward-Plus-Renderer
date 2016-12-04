// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include <glm/glm.hpp>

#include <memory>

class GLFWwindow;
class _VulkanContext_Impl;

class VulkanContext
{
public:
	VulkanContext(GLFWwindow* window);
	~VulkanContext();

	void resize(int width, int height);
	void requestDraw(float deltatime);
	void cleanUp();

	void setCamera(const glm::mat4 & view, const glm::vec3 campos);

	VulkanContext(const VulkanContext&) = delete;
	VulkanContext& operator= (const VulkanContext&) = delete;
	VulkanContext(VulkanContext&&) = delete;
	VulkanContext& operator= (VulkanContext&&) = delete;

private:
	std::unique_ptr<_VulkanContext_Impl> p_impl;
};