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

//// for test use
//TestSceneConfiguration small_sponza
//{
//	util::getContentPath("sponza/sponza.obj"),  //model_file
//	1.0f,  // scale
//	glm::vec3 { -15, -5, -5 },  // min_light_pos
//	glm::vec3 { 15, 20, 5 },  // max_light_pos
//	5.0f,  // radius
//	200  // light num
//};

// for test use
TestSceneConfiguration sponza_full_5_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	5.0f,  // radius
	5  // light num
};

TestSceneConfiguration sponza_full_200_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	5.0f,  // radius
	200  // light num
};

TestSceneConfiguration sponza_full_1000_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	5.0f,  // radius
	1000  // light num
};

TestSceneConfiguration sponza_full_1000_small_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	2.0f,  // radius
	1000  // light num
};

TestSceneConfiguration sponza_full_1000_large_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	10.0f,  // radius
	1000  // light num
};

TestSceneConfiguration sponza_full_10000_small_lights
{
	util::getContentPath("sponza_full/sponza.obj"),  //model_file
	0.01f,  // scale
	glm::vec3{ -15, -5, -5 },  // min_light_pos
	glm::vec3{ 15, 20, 5 },  // max_light_pos
	1.0f,  // radius
	10000  // light num
};


TestSceneConfiguration rungholt_20_lights
{
	util::getContentPath("rungholt/rungholt.obj"),  //model_file
	0.10f,  // scale
	glm::vec3{ -20, -5, -20 },  // min_light_pos
	glm::vec3{ 20, 20, 20 },  // max_light_pos
	10.0f,  // radius
	20  // light num
};


TestSceneConfiguration rungholt_1000_lights
{
	util::getContentPath("rungholt/rungholt.obj"),  //model_file
	0.10f,  // scale
	glm::vec3{ -20, -5, -20 },  // min_light_pos
	glm::vec3{ 20, 20, 20 },  // max_light_pos
	5.0f,  // radius
	1000  // light num
};

int main()
{
	auto result = EXIT_SUCCESS;

	// change this to test different scenes
	getGlobalTestSceneConfiguration() = sponza_full_200_lights;

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
