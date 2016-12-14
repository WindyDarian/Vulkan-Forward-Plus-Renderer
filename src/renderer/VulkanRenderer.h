// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include <glm/glm.hpp>

#include <memory>

struct GLFWwindow;
class _VulkanRenderer_Impl;

class VulkanRenderer
{
public:
	VulkanRenderer(GLFWwindow* window);
	~VulkanRenderer();

	int getDebugViewIndex() const;

	void resize(int width, int height);
	void changeDebugViewIndex(int target_view);
	void requestDraw(float deltatime);
	void cleanUp();

	void setCamera(const glm::mat4 & view, const glm::vec3 campos);

	VulkanRenderer(const VulkanRenderer&) = delete;
	VulkanRenderer& operator= (const VulkanRenderer&) = delete;
	VulkanRenderer(VulkanRenderer&&) = delete;
	VulkanRenderer& operator= (VulkanRenderer&&) = delete;

private:
	std::unique_ptr<_VulkanRenderer_Impl> p_impl;
};