// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "ShowBase.h"

#include "renderer/VulkanContext.h"
#include "util.h"

#include <GLFW/glfw3.h>

#include <chrono>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int WINDOW_WIDTH = 1920;
constexpr int WINDOW_HEIGHT = 1080;
constexpr bool WINDOW_RESIZABLE = true;
constexpr float MIN_DELTA_TIME = 1.0f / 60.0f;

struct Camera
{
public:
	glm::vec3 position = { 1.5f, 1.5f, 1.5f };
	glm::quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
	float rotation_speed = glm::pi<float>();

	//float fov;
	//glm::vec2 near;
	//glm::vec2 far;

	glm::mat4 getViewMatrix() const
	{
		// return glm::inverse(glm::translate(glm::toMat4(rotation), position));
		return glm::transpose(glm::toMat4(rotation)) * glm::translate(glm::mat4(1.0f), -position); // equal to inv(TR) 
	}
};

class _ShowBase_Impl
{
private:
	GLFWwindow* window = createWindow();
	VulkanContext context{window};
	Camera camera;
	// for measurement
	float total_time_past = 0.0f;
	int total_frames = 0;
	//float delta_time = 0.0f;

	bool lmb_down = false;
	bool rmb_down = false;
	glm::vec2 cursor_pos = { 0.0f, 0.0f };
	glm::vec2 cursor_pos_on_button_down = { 0.0f, 0.0f };
	glm::vec2 framebuffer_size = { WINDOW_WIDTH, WINDOW_HEIGHT };
	
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
	
		auto buffersize_callback = [](GLFWwindow* window, int width, int height)
		{
			auto app = reinterpret_cast<_ShowBase_Impl*>(glfwGetWindowUserPointer(window));
			app->onResize(width, height);
		};

		auto cursorpos_callback = [](GLFWwindow* window, double xPos, double yPos)
		{
			auto app = reinterpret_cast<_ShowBase_Impl*>(glfwGetWindowUserPointer(window));
			app->onCursorPosChanged(xPos, yPos);
		};

		auto mousebutton_callback = [](GLFWwindow* window, int button, int action, int mods)
		{
			auto app = reinterpret_cast<_ShowBase_Impl*>(glfwGetWindowUserPointer(window));
			app->onMouseButton(button, action, mods);
		};

		glfwSetFramebufferSizeCallback(window, buffersize_callback);
		glfwSetCursorPosCallback(window, cursorpos_callback);
		glfwSetMouseButtonCallback(window, mousebutton_callback);

		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		framebuffer_size = { w, h };

		//glfwSetKeyCallback(window, keyCallback);
		//glfwSetScrollCallback(window, scrollCallback);

		return window;
	}

	void tick(float delta_time)
	{
		if (rmb_down)
		{
			if (util::isNearlyEqual(framebuffer_size.x, 0.f) || util::isNearlyEqual(framebuffer_size.x, 0.f))
			{
				return;
			}

			auto cursor_delta = (cursor_pos - cursor_pos_on_button_down) / glm::min(framebuffer_size.x, framebuffer_size.y) * 2.0f;

			if (!util::isNearlyEqual(cursor_delta.x, 0))
			{
				//camera.rotation = camera.rotation * glm::angleAxis(camera.rotation_speed * cursor_delta.x, util::vec_up);
				camera.rotation = glm::angleAxis(camera.rotation_speed * cursor_delta.x, util::vec_up) * camera.rotation; // world up
			}

			if (!util::isNearlyEqual(cursor_delta.y, 0))
			{
				//camera.rotation = camera.rotation * glm::angleAxis(camera.rotation_speed * cursor_delta.y, util::vec_right);
				camera.rotation = camera.rotation * glm::angleAxis(camera.rotation_speed * cursor_delta.y, util::vec_right); // local right
			}

			glfwSetCursorPos(window, cursor_pos_on_button_down.x, cursor_pos_on_button_down.y);
		}
	}

	void mainLoop()
	{
		auto start_time = std::chrono::high_resolution_clock::now();
		auto previous = std::chrono::high_resolution_clock::now();
		decltype(previous) current;
		float delta_time;

		while (!glfwWindowShouldClose(window))
		{
			current = std::chrono::high_resolution_clock::now();
			delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(current - previous).count() / 1000.0f;

			glfwPollEvents();

			if (delta_time >= MIN_DELTA_TIME) //prevent underflow
			{
				tick(delta_time);
				previous = current;
			}

			context.setViewMatrix(camera.getViewMatrix());
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


	void onResize(int width, int height)
	{
		if (width == 0 || height == 0) return;
		context.resize(width, height);
		framebuffer_size = { width, height };
	}

	void onMouseButton(int button, int action, int mods)
	{
		if (action == GLFW_PRESS) {
			double x, y;
			glfwGetCursorPos(window, &x, &y);
			cursor_pos = {x, y};
			cursor_pos_on_button_down = {x, y};

			if (button == GLFW_MOUSE_BUTTON_LEFT)
			{
				lmb_down = true;
			}
			else if (button == GLFW_MOUSE_BUTTON_RIGHT)
			{
				rmb_down = true;
			}
		}
		else if (action == GLFW_RELEASE)
		{
			if (button == GLFW_MOUSE_BUTTON_LEFT)
			{
				lmb_down = false;
			}
			else if (button == GLFW_MOUSE_BUTTON_RIGHT)
			{
				rmb_down = false;
			}
		}
	}

	void onCursorPosChanged(double xPos, double yPos) 
	{
		//std::cout << xPos << "," << yPos << std::endl;
		if (!lmb_down && !rmb_down) {
			return;
		}
		else if (lmb_down) {
			//glm::vec2 tmp(xPos, yPos);
			//modelRotAngles += (tmp - cursorPos) * 0.01f;

			cursor_pos = { xPos, yPos };
		}
		else if (rmb_down) {
			glm::vec2 tmp(xPos, yPos);

			cursor_pos = { xPos, yPos };
		}

	}

public:
	_ShowBase_Impl()
	{

	}

	~_ShowBase_Impl()
	{
		glfwSetFramebufferSizeCallback(window, nullptr);
		glfwSetCursorPosCallback(window, nullptr);
		glfwSetMouseButtonCallback(window, nullptr);
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
