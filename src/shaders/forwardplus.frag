#version 450
#extension GL_ARB_separate_shader_objects : enable

const int TILE_SIZE = 16;

struct PointLight {
	vec3 pos;
	float radius;
	vec3 intensity;
};

#define MAX_POINT_LIGHT_PER_TILE 1023
struct LightVisiblity
{
	uint count;
	uint lightindices[MAX_POINT_LIGHT_PER_TILE];
};

layout(push_constant) uniform PushConstantObject
{
	ivec2 viewport_size;
	ivec2 tile_nums;
    int debugview_index;
} push_constants;

layout(std140, set = 0, binding = 0) uniform SceneObjectUbo
{
    mat4 model;
} transform;

// layout(set = 0, binding = 1) uniform sampler2D tex_sampler;
// layout(set = 0, binding = 2) uniform sampler2D normal_sampler;

layout(std140, set = 1, binding = 0) buffer readonly CameraUbo // FIXME: change back to uniform
{
    mat4 view;
    mat4 proj;
    mat4 projview;
    vec3 cam_pos;
} camera;

layout(std430, set = 2, binding = 0) buffer readonly TileLightVisiblities
{
    LightVisiblity light_visiblities[];
};

layout(std140, set = 2, binding = 1) buffer readonly PointLights // FIXME: change back to uniform // readonly buffer PointLights
{
	int light_num;
	PointLight pointlights[20000];
};

layout(set = 3, binding = 0) uniform sampler2D depth_sampler;

layout(std140, set = 4, binding = 0) uniform MaterialUbo
{
    int has_albedo_map;
    int has_normal_map;
} material;

layout(set = 4, binding = 1) uniform sampler2D albedo_sampler;
layout(set = 4, binding = 2) uniform sampler2D normal_sampler;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_tex_coord;
layout(location = 2) in vec3 frag_normal;
layout(location = 3) in vec3 frag_pos_world;

layout(location = 0) out vec4 out_color;

layout(early_fragment_tests) in; // for early depth test

vec3 applyNormalMap(vec3 geomnor, vec3 normap)
{
    normap = normap * 2.0 - 1.0;
    vec3 up = normalize(vec3(0.001, 1, 0.001));
    vec3 surftan = normalize(cross(geomnor, up));
    vec3 surfbinor = cross(geomnor, surftan);
    return normalize(normap.y * surftan + normap.x * surfbinor + normap.z * geomnor);
}

void main()
{

    vec3 diffuse;
    if (material.has_albedo_map > 0)
    {
        diffuse = texture(albedo_sampler, frag_tex_coord).rgb;
    }
    else
    {
        diffuse = vec3(1.0);
    }

    vec3 normal;
    if (material.has_normal_map > 0)
    {
        normal = applyNormalMap(frag_normal, texture(normal_sampler, frag_tex_coord).rgb);
    }
    else
    {
        normal = frag_normal;
    }
    ivec2 tile_id = ivec2(gl_FragCoord.xy / TILE_SIZE);
    uint tile_index = tile_id.y * push_constants.tile_nums.x + tile_id.x;

    // debug view
    if (push_constants.debugview_index > 1)
    {
        if (push_constants.debugview_index == 2)
        {
			//heat map debug view
			float intensity = float(light_visiblities[tile_index].count) / 64;
            out_color = vec4(vec3(intensity), 1.0) ; //light culling debug
			//out_color = vec4(vec3(intensity * 0.62, intensity * 0.13, intensity * 0.94), 1.0) ; //light culling debug
			//float minimum = 0.0;
			//float maximum = 1.0;
			//float ratio = 2 * (intensity - minimum) / (maximum - minimum);
			//float b = max(0, 1 - ratio);
			//float r = max(0, ratio - 1);
			//float g = max(0, 1.0 - b - r);
		        //out_color = vec4(vec3(r,g,b), 1.0);
		}
		else if (push_constants.debugview_index == 3)
        {
            // depth debug view
            float pre_depth = texture(depth_sampler, (gl_FragCoord.xy/push_constants.viewport_size) ).x;
            out_color = vec4(vec3( pre_depth ),1.0);
        }
        else if (push_constants.debugview_index == 4)
        {
            // normal debug view
            out_color = vec4(abs(normal), 1.0);
        }
        return;
    }


    vec3 illuminance = vec3(0.0);
    uint tile_light_num = light_visiblities[tile_index].count;
    for (int i = 0; i < tile_light_num; i++)
	{
        PointLight light = pointlights[light_visiblities[tile_index].lightindices[i]];
		vec3 light_dir = normalize(light.pos - frag_pos_world);
        float lambertian = max(dot(light_dir, normal), 0.0);

        if(lambertian > 0.0)
        {
            float light_distance = distance(light.pos, frag_pos_world);
            if (light_distance > light.radius)
            {
                continue;
            }

            vec3 viewDir = normalize(camera.cam_pos - frag_pos_world);
            vec3 halfDir = normalize(light_dir + viewDir);
            float specAngle = max(dot(halfDir, normal), 0.0);
            float specular = pow(specAngle, 32.0);  // TODO?: spec color & power in g-buffer?

            float att = clamp(1.0 - light_distance * light_distance / (light.radius * light.radius), 0.0, 1.0);
            illuminance += light.intensity * att * (lambertian * diffuse + specular);
        }
	}

    //heat map with render debug view
    if (push_constants.debugview_index == 1)
    {
        float intensity = float(light_visiblities[tile_index].count) / (64 / 2.0);
        out_color = vec4(vec3(intensity, intensity * 0.5, intensity * 0.5) + illuminance * 0.25, 1.0) ; //light culling debug
        return;
    }

    // render view
    out_color = vec4(illuminance, 1.0);

    //out_color = vec4(0.0, 0.0, 0.0, 1.0);
    //out_color[light_visiblities[tile_index].count] = 1.0;
    //out_color = vec4(illuminance, 1.0);
    //out_color = vec4(abs(normal), 1.0);
    //out_color = vec4(abs(texture(normal_sampler, frag_tex_coord).rgb), 1.0); // normal map debug view
}
