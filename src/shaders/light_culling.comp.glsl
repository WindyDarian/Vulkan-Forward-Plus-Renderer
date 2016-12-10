#version 450
#extension GL_ARB_separate_shader_objects : enable

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

layout(std430, set = 0, binding = 0) buffer writeonly TileLightVisiblities
{
    LightVisiblity light_visiblities[];
};

layout(std140, set = 0, binding = 1) uniform PointLights // readonly buffer PointLights
{
	int light_num;
	PointLight pointlights[1000];
};

void main()
{
    // wild work in progress!
	ivec2 tile_id = ivec2(gl_WorkGroupID.xy);
	ivec2 tile_nums = ivec2(gl_NumWorkGroups.xy);
	uint tile_index = tile_id.y * tile_nums.x + tile_id.x;
	light_visiblities[tile_index].count = int(float(tile_index)) % 7;
}
