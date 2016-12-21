// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // opengl's depth range was -1 to 1
#define GLFW_INCLUDE_VULKAN

#include "ShowBase.h"
#include "scene.h" // For benchmark
#include "util.h"

#include <stdexcept>
#include <iostream>
#include <string>

// for test use
TestSceneConfiguration sponza_full_10_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	5.0f,  // radius
	10,  // light num
	glm::vec3{ 12.7101822f, 1.87933588f, -0.0333303586f },  // camera position
	glm::quat{ 0.717312694f, -0.00208670134f, 0.696745396f, 0.00202676491f }   // camera rotation
};

TestSceneConfiguration sponza_full_200_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	5.0f,  // radius
	200,  // light num
	glm::vec3{ 12.7101822f, 1.87933588f, -0.0333303586f },  // camera position
	glm::quat{ 0.717312694f, -0.00208670134f, 0.696745396f, 0.00202676491f }   // camera rotation
};

TestSceneConfiguration sponza_full_1000_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	5.0f,  // radius
	1000,  // light num
	glm::vec3{ 12.7101822f, 1.87933588f, -0.0333303586f },  // camera position
	glm::quat{ 0.717312694f, -0.00208670134f, 0.696745396f, 0.00202676491f }   // camera rotation
};

TestSceneConfiguration sponza_full_1000_small_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	2.0f,  // radius
	1000,  // light num
	glm::vec3{ 12.7101822f, 1.87933588f, -0.0333303586f },  // camera position
	glm::quat{ 0.717312694f, -0.00208670134f, 0.696745396f, 0.00202676491f }   // camera rotation
};

TestSceneConfiguration sponza_full_1000_large_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	10.0f,  // radius
	1000,  // light num
	glm::vec3{ 12.7101822f, 1.87933588f, -0.0333303586f },  // camera position
	glm::quat{ 0.717312694f, -0.00208670134f, 0.696745396f, 0.00202676491f }   // camera rotation
};

TestSceneConfiguration sponza_full_20000_small_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	2.0f,  // radius
	20000,  // light num
	glm::vec3{ 12.7101822f, 1.87933588f, -0.0333303586f },  // camera position
	glm::quat{ 0.717312694f, -0.00208670134f, 0.696745396f, 0.00202676491f }   // camera rotation
};

TestSceneConfiguration rungholt_10_lights
{
	util::getContentPath("rungholt/rungholt.obj"),  //model_file
	0.10f,  // scale
	glm::vec3{ -20, -5, -20 },  // min_light_pos
	glm::vec3{ 20, 20, 20 },  // max_light_pos
	10.0f,  // radius
	10,  // light num
	glm::vec3{ 31.8083534f, 27.5098400f, 36.7743378f },  // camera position
	glm::quat{ 0.883192122f, -0.292658001f, 0.347898334f, 0.115281112f  }   // camera rotation
};

TestSceneConfiguration rungholt_200_lights
{
	util::getContentPath("rungholt/rungholt.obj"),  //model_file
	0.10f,  // scale
	glm::vec3{ -20, -5, -20 },  // min_light_pos
	glm::vec3{ 20, 20, 20 },  // max_light_pos
	5.0f,  // radius
	200,  // light num
	glm::vec3{ 31.8083534f, 27.5098400f, 36.7743378f },  // camera position
	glm::quat{ 0.883192122f, -0.292658001f, 0.347898334f, 0.115281112f }   // camera rotation
};

TestSceneConfiguration rungholt_1000_lights
{
	util::getContentPath("rungholt/rungholt.obj"),  //model_file
	0.10f,  // scale
	glm::vec3{ -20, -5, -20 },  // min_light_pos
	glm::vec3{ 20, 20, 20 },  // max_light_pos
	5.0f,  // radius
	1000,  // light num
	glm::vec3{ 31.8083534f, 27.5098400f, 36.7743378f },  // camera position
	glm::quat{ 0.883192122f, -0.292658001f, 0.347898334f, 0.115281112f }   // camera rotation
};

TestSceneConfiguration rungholt_20000_lights
{
	util::getContentPath("rungholt/rungholt.obj"),  //model_file
	0.10f,  // scale
	glm::vec3{ -20, -5, -20 },  // min_light_pos
	glm::vec3{ 20, 20, 20 },  // max_light_pos
	5.0f,  // radius
	20000,  // light num
	glm::vec3{ 31.8083534f, 27.5098400f, 36.7743378f },  // camera position
	glm::quat{ 0.883192122f, -0.292658001f, 0.347898334f, 0.115281112f }   // camera rotation
};

int main()
{
	auto result = EXIT_SUCCESS;

	// change this to test different scenes
	getGlobalTestSceneConfiguration() = sponza_full_1000_small_lights;

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
