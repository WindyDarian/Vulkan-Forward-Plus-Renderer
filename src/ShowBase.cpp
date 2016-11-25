// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "ShowBase.h"

#include "renderer/VulkanContext.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const bool WINDOW_RESIZABLE = true;

class _ShowBase_Impl
{
private:
	GLFWwindow* window = createWindow();
	VulkanContext context{window};

	// for measurement
	float total_time_past = 0.0f;
	int total_frames = 0;
	
	GLFWwindow* createWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // no OpenGL context

		if (WINDOW_RESIZABLE)
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		}
		else
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		}

		auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Hello World", nullptr, nullptr);

		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, _ShowBase_Impl::onResize);

		return window;
	}

	static void onResize(GLFWwindow* window, int width, int height)
	{
		if (width == 0 || height == 0) return;

		auto app = reinterpret_cast<_ShowBase_Impl*>(glfwGetWindowUserPointer(window));
		app->context.resize(width, height);
	}

	void mainLoop()
	{
		auto start_time = std::chrono::high_resolution_clock::now();
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			context.requestDraw();
			total_frames++;
		}
		auto end_time = std::chrono::high_resolution_clock::now();
		total_time_past = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0f;
		if (total_time_past > 0)
		{
			std::cout << "FPS: " << total_frames / total_time_past << std::endl;
		}
		context.cleanUp();
	}

public:
	_ShowBase_Impl()
	{

	}

	void run()
	{
		mainLoop();
	}
};

void ShowBase::run()
{
	p_impl->run();
}

ShowBase::ShowBase()
{
	p_impl = std::make_unique<_ShowBase_Impl>();
}

ShowBase::~ShowBase() = default; 
