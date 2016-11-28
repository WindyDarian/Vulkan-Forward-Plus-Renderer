// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// TODO: now the scene is just a low level struct... once renderer is basically finished 
//       and we start working on scene management there should be a higher level scene class
//      (and there should be some change in dependency relationship.... or just leave this Scene stuct as some RenderableScene)

struct PointLight
{
public:
	//glm::vec3 pos = { 0.0f, 1.0f, 0.0f };
	glm::vec3 pos;
	float radius = { 5.0f };
	glm::vec3 intensity = { 1.0f, 1.0f, 1.0f };
	float padding;

	PointLight() {}
	PointLight(glm::vec3 pos, float radius, glm::vec3 intensity)
		: pos(pos), radius(radius), intensity(intensity)
	{};
};

struct Camera
{
public:
	glm::vec3 position = { 1.5f, 1.5f, 1.5f };
	glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
	float rotation_speed = glm::pi<float>();
	float move_speed = 10.f;

	//float fov;
	//glm::vec2 near;
	//glm::vec2 far;

	glm::mat4 getViewMatrix() const
	{
		// return glm::inverse(glm::translate(glm::toMat4(rotation), position));
		return glm::transpose(glm::toMat4(rotation)) * glm::translate(glm::mat4(1.0f), -position); // equal to inv(TR) 
	}
};

struct Scene
{
	std::vector<PointLight> point_lights{ PointLight() }; // just one light for test
	Camera camera;
};