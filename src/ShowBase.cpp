// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "ShowBase.h"

#include "renderer/VulkanRenderer.h"
#include "scene.h"
#include "util.h"

#include <GLFW/glfw3.h>

#include <chrono>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

constexpr int WINDOW_WIDTH = 1920;
constexpr int WINDOW_HEIGHT = 1080;
constexpr bool WINDOW_RESIZABLE = true;
constexpr float MIN_DELTA_TIME = 1.0f / 60.0f;

class _ShowBase_Impl
{
private:
	GLFWwindow* window = createWindow();
	VulkanRenderer renderer{window};
	Camera camera;
	// for measurement
	float total_time_past = 0.0f;
	int total_frames = 0;
	//float delta_time = 0.0f;

	bool lmb_down = false;
	bool rmb_down = false;
	glm::vec2 cursor_pos = { 0.0f, 0.0f };
	glm::vec2 prev_cursor_pos = { 0.0f, 0.0f };
	glm::vec2 framebuffer_size = { WINDOW_WIDTH, WINDOW_HEIGHT };
	bool w_down = false; // todo use a hash map or something
	bool s_down = false;
	bool a_down = false;
	bool d_down = false;
	bool q_down = false;
	bool e_down = false;
	bool z_pressed = false;


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

		auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "VFPR - Vulkan Forward+ Renderer", nullptr, nullptr);
		
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

		auto key_callback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			auto app = reinterpret_cast<_ShowBase_Impl*>(glfwGetWindowUserPointer(window));
			app->onKeyPress(key, scancode, action, mods);
		};

		glfwSetFramebufferSizeCallback(window, buffersize_callback);
		glfwSetCursorPosCallback(window, cursorpos_callback);
		glfwSetMouseButtonCallback(window, mousebutton_callback);

		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		framebuffer_size = { w, h };

		glfwSetKeyCallback(window, key_callback);
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

			auto cursor_delta = (cursor_pos - prev_cursor_pos) / glm::min(framebuffer_size.x, framebuffer_size.y) * 2.0f;

			if (!util::isNearlyEqual(cursor_delta.x, 0))
			{
				camera.rotation = glm::angleAxis(camera.rotation_speed * -cursor_delta.x, util::vec_up) * camera.rotation; // world up
			}

			if (!util::isNearlyEqual(cursor_delta.y, 0))
			{
				camera.rotation = camera.rotation * glm::angleAxis(camera.rotation_speed * -cursor_delta.y, util::vec_right); // local right
			}

			camera.rotation = glm::normalize(camera.rotation);

			prev_cursor_pos = cursor_pos;
		}

		if (w_down) 
		{
			// forward
			camera.position += camera.rotation  * util::vec_forward * camera.move_speed * delta_time;
		}

		if (s_down) 
		{
			// back
			camera.position -= camera.rotation  * util::vec_forward * camera.move_speed * delta_time;
		}

		if (a_down)
		{
			// left
			camera.position -= camera.rotation  * util::vec_right * camera.move_speed * delta_time;
		}

		if (d_down)
		{
			// right
			camera.position += camera.rotation  * util::vec_right * camera.move_speed * delta_time;
		}

		if (q_down)
		{
			// up
			camera.position -= util::vec_up * camera.move_speed * delta_time;
		}

		if (e_down)
		{
			// down
			camera.position += util::vec_up * camera.move_speed * delta_time;
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
			delta_time = std::chrono::duration<float>(current - previous).count();

			glfwPollEvents();

			if (z_pressed) // change debug view
			{
				z_pressed = false;
				renderer.changeDebugViewIndex(renderer.getDebugViewIndex() + 1);
			}

			if (delta_time >= MIN_DELTA_TIME) //prevent underflow
			{
				tick(delta_time);
				previous = current;
			}

			renderer.setCamera(camera.getViewMatrix(), camera.position);
			renderer.requestDraw(delta_time);
			total_frames++;

		}
		auto end_time = std::chrono::high_resolution_clock::now();
		total_time_past = std::chrono::duration<float>(end_time - start_time).count();
		if (total_time_past > 0)
		{
			std::cout << "FPS: " << total_frames / total_time_past << std::endl;
		}
		renderer.cleanUp();
	}


	void onResize(int width, int height)
	{
		if (width == 0 || height == 0) return;
		renderer.resize(width, height);
		framebuffer_size = { width, height };
	}

	void onMouseButton(int button, int action, int mods)
	{
		if (action == GLFW_PRESS) {
			double x, y;
			glfwGetCursorPos(window, &x, &y);
			cursor_pos = {x, y};
			prev_cursor_pos = {x, y};

			if (button == GLFW_MOUSE_BUTTON_LEFT)
			{
				lmb_down = true;
			}
			else if (button == GLFW_MOUSE_BUTTON_RIGHT)
			{
				rmb_down = true;
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}
	}

	void onKeyPress(int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS) 
		{
			switch (key) 
			{
				// todo use a hash map or something
			    case GLFW_KEY_W:
				    w_down = true;
					break;
			    case GLFW_KEY_S:
				    s_down = true;
					break;
				case GLFW_KEY_A:
					a_down = true;
					break;
				case GLFW_KEY_D:
					d_down = true;
					break;
				case GLFW_KEY_Q:
					q_down = true;
					break;
				case GLFW_KEY_E:
					e_down = true;
					break;
			}
		}
		else if (action == GLFW_RELEASE) 
		{
			switch (key) 
			{
			    case GLFW_KEY_W:
				    w_down = false;
					break;
			    case GLFW_KEY_S:
				    s_down = false;
					break;
				case GLFW_KEY_A:
					a_down = false;
					break;
				case GLFW_KEY_D:
					d_down = false;
					break;
				case GLFW_KEY_Q:
					q_down = false;
					break;
				case GLFW_KEY_E:
					e_down = false;
					break;
				case GLFW_KEY_Z:
					z_pressed = true;
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
		camera.position = getGlobalTestSceneConfiguration().camera_position;
		camera.rotation = glm::normalize(getGlobalTestSceneConfiguration().camera_rotation);
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
	: p_impl(std::make_unique<_ShowBase_Impl>())
{}

ShowBase::~ShowBase() = default; 
