#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

const int TILE_SIZE = 16; // TODO: maybe I can use push constant?

struct PointLight {
	vec3 pos;
	float radius;
	vec3 intensity;
};

#define MAX_POINT_LIGHT_PER_TILE 63

struct LightVisiblity
{
	int count;
	int lightindices[MAX_POINT_LIGHT_PER_TILE];
};

layout(push_constant) uniform PushConstantObject 
{
	ivec2 viewport_size;
	ivec2 tile_nums;
} push_constants;

layout(std430, set = 0, binding = 0) buffer writeonly TileLightVisiblities
{
    LightVisiblity light_visiblities[];
};

layout(std140, set = 0, binding = 1) buffer readonly PointLights // FIXME: change back to uniform
{
	int light_num;
	PointLight pointlights[1000];
};

layout(std140, set = 1, binding = 0) buffer readonly CameraUbo // FIXME: change back to uniform
{
    mat4 view;
    mat4 proj;
    mat4 projview;
    vec3 cam_pos;
} camera;

// vulkan ndc, minDepth = 0.0, maxDepth = 1.0
const vec3 ndc_upper_left_near = vec3(-1.0, -1.0, 0.0);
const vec3 ndc_upper_right_near = vec3(1.0, -1.0, 0.0);
const vec3 ndc_lower_left_near = vec3(-1.0, 1.0, 0.0);
const vec3 ndc_lower_right_near = vec3(1.0, 1.0, 0.0);
const vec3 ndc_upper_left_far = vec3(-1.0, -1.0, 1.0);
const vec3 ndc_upper_right_far = vec3(1.0, -1.0, 1.0);
const vec3 ndc_lower_left_far = vec3(-1.0, 1.0, 1.0);
const vec3 ndc_lower_right_far = vec3(1.0, 1.0, 1.0);
const float ndc_near_plane = 0.0;
const float ndc_far_plane = 1.0;

shared mat4 inv_projview;
shared float ada[16];

layout(local_size_x = 1 + MAX_POINT_LIGHT_PER_TILE) in;

void main()
{
    // wild work in progress!
	ivec2 tile_id = ivec2(gl_WorkGroupID.xy);
	//ivec2 tile_nums = ivec2(gl_NumWorkGroups.xy);
	uint tile_index = tile_id.y * push_constants.tile_nums.x + tile_id.x;

	light_visiblities[tile_index].count = int(tile_index + light_num) % 3;

	// TODO: depth culling??? (do i need to read depth output of vertex shader?????)
	
	// Construct frustum planes
	if (gl_LocalInvocationIndex == 0) {
		//inv_projview = inverse(camera.projview); // crash
		//inv_projview = camera.projview; // crash
		// for (int i = 0; i < 16; i++) 
		// {
		// 	ada[i] = camera.projview[i/4][i%4]; // crash
		// }
		// for (int i = 0; i < 16; i++) 
		// {
		// 	ada[i] = i; // work
		// }
		// ada[0] = camera.projview[0][0]; // crash
		ada[0] = 1;
		
		// wtf
		// TODO

	}
	barrier();
}
