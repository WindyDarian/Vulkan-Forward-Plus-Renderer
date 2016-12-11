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
const vec2 ndc_upper_left = vec2(-1.0, -1.0);
const float ndc_near_plane = 0.0;
const float ndc_far_plane = 1.0;

shared mat4 inv_projview;
shared float ada[16];

//layout(local_size_x = 1 + MAX_POINT_LIGHT_PER_TILE) in; //TODO

struct ViewFrustum
{
	vec4 planes[6];
	vec3 points[8]; // 0-3 near 4-7 far
};

// Construct view frustum 
ViewFrustum createFrustum(ivec2 tile_id)
{
	inv_projview = inverse(camera.projview); 

	vec2 ndc_size_per_tile = 2.0 * vec2(TILE_SIZE, TILE_SIZE) / push_constants.viewport_size;
	
	vec2 ndc_pts[4];  // corners of tile in ndc
	ndc_pts[0] = ndc_upper_left + tile_id * ndc_size_per_tile;  // upper left
	ndc_pts[1] = vec2(ndc_pts[0].x + ndc_size_per_tile.x, ndc_pts[0].y); // upper right
	ndc_pts[2] = vec2(ndc_pts[0].x, ndc_pts[0].y + ndc_size_per_tile.x); // lower left
	ndc_pts[3] = ndc_pts[0] + ndc_size_per_tile;
	
	ViewFrustum frustum;
	
	vec4 temp;
	for (int i = 0; i < 4; i++)
	{
		temp = inv_projview * vec4(ndc_pts[i], ndc_near_plane, 1.0);
		frustum.points[i] = temp.xyz / temp.w;
		temp = inv_projview * vec4(ndc_pts[i], ndc_far_plane, 1.0);
		frustum.points[i + 4] = temp.xyz / temp.w;
	}

	vec3 temp_normal;
	for (int i = 0; i < 4; i++)
	{
		//Cax+Cby+Ccz+Cd = 0, planes[i] = (Ca, Cb, Cc, Cd)
		// temp_normal: normal without normalization
		temp_normal = cross(frustum.points[i] - camera.cam_pos, frustum.points[i + 1] - camera.cam_pos); 
		frustum.planes[i] = vec4(temp_normal, - dot(temp_normal, frustum.points[i]));
	}
	// near plane
	{
		temp_normal = cross(frustum.points[1] - frustum.points[0], frustum.points[3] - frustum.points[0]); 
		frustum.planes[4] = vec4(temp_normal, - dot(temp_normal, frustum.points[0]));
	}
	// far plane
	{
		temp_normal = cross(frustum.points[7] - frustum.points[4], frustum.points[5] - frustum.points[4]); 
		frustum.planes[5] = vec4(temp_normal, - dot(temp_normal, frustum.points[4]));
	}

	return frustum;
} 

bool isCollided(PointLight light, ViewFrustum frustum)
{
	vec3 light_bbox_max = light.pos + vec3(light.radius);
	vec3 light_bbox_min = light.pos - vec3(light.radius);

	// ref: http://www.iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
	// check box outside/inside of frustum
    for(int i=0; i<6; i++)
    {
        int probe = 0;
        probe += ((dot( frustum.planes[i], vec4(light_bbox_min.x, light_bbox_min.y, light_bbox_min.z, 1.0) ) < 0.0 )?1:0);
        probe += ((dot( frustum.planes[i], vec4(light_bbox_max.x, light_bbox_min.y, light_bbox_min.z, 1.0) ) < 0.0 )?1:0);
        probe += ((dot( frustum.planes[i], vec4(light_bbox_min.x, light_bbox_max.y, light_bbox_min.z, 1.0) ) < 0.0 )?1:0);
        probe += ((dot( frustum.planes[i], vec4(light_bbox_max.x, light_bbox_max.y, light_bbox_min.z, 1.0) ) < 0.0 )?1:0);
        probe += ((dot( frustum.planes[i], vec4(light_bbox_min.x, light_bbox_min.y, light_bbox_max.z, 1.0) ) < 0.0 )?1:0);
        probe += ((dot( frustum.planes[i], vec4(light_bbox_max.x, light_bbox_min.y, light_bbox_max.z, 1.0) ) < 0.0 )?1:0);
        probe += ((dot( frustum.planes[i], vec4(light_bbox_min.x, light_bbox_max.y, light_bbox_max.z, 1.0) ) < 0.0 )?1:0);
        probe += ((dot( frustum.planes[i], vec4(light_bbox_max.x, light_bbox_max.y, light_bbox_max.z, 1.0) ) < 0.0 )?1:0);
        if( probe ==8 ) return false;
    }

    return true;
}

void main()
{
    // wild work in progress!
	ivec2 tile_id = ivec2(gl_WorkGroupID.xy);
	//ivec2 tile_nums = ivec2(gl_NumWorkGroups.xy);
	uint tile_index = tile_id.y * push_constants.tile_nums.x + tile_id.x;

	

	// TODO: depth culling??? (do i need to read depth output of vertex shader?????)
	

	//if (gl_LocalInvocationIndex == 0) { // TODO: local multithread

	ViewFrustum frustum = createFrustum(tile_id);

	int light_count_for_tile = 0;
	for (int i = 0; i < light_num && light_count_for_tile < MAX_POINT_LIGHT_PER_TILE; i++)
	{
		if (isCollided(pointlights[i], frustum))
		{
			light_visiblities[tile_index].lightindices[light_count_for_tile] = i;
			light_count_for_tile++;
		}
	}

	light_visiblities[tile_index].count = light_count_for_tile;

	//}
	//barrier();
}
