// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // opengl's depth range was -1 to 1
#define GLFW_INCLUDE_VULKAN

#include "ShowBase.h"

#include <stdexcept>
#include <iostream>

int main()
{
	auto result = EXIT_SUCCESS;

	try
	{
		ShowBase app;
		app.run();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;

		result = EXIT_FAILURE;
	} 

	//system("pause");
	return result;
}
