#version 450
#extension GL_ARB_separate_shader_objects : enable

// layout(set = 0, binding = 0) uniform UniformBufferObject
// {
//     mat4 model;
//     mat4 view;
//     mat4 proj;
// } transform;

struct PointLight {
	vec3 pos;
	float radius;
	vec3 intensity;
};

layout(set = 0, binding = 1) uniform sampler2D tex_sampler;
layout(std430, set = 0, binding = 2) readonly buffer PointLights 
{
	PointLight pointlights[2];
};

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_tex_coord;
layout(location = 2) in vec3 frag_normal;
layout(location = 3) in vec3 frag_pos_world;

layout(location = 0) out vec4 out_color;

const vec3 light_direction = vec3(1.0, 1.0, 1.0);

void main()
{
    vec4 color_map =  texture(tex_sampler, frag_tex_coord);
    vec3 texcolor = color_map.rgb;
    vec3 illuminance = vec3(0.0);

	// vec3 light_dir = normalize(pointlights[0].pos - frag_pos_world);
    // float lambertian = max(dot(light_dir, frag_normal), 0.0);
    // float light_distance = distance(pointlights[0].pos, frag_pos_world);
    // float att = clamp(1.0 - light_distance * light_distance / (pointlights[0].radius * pointlights[0].radius), 0.0, 1.0);
    // illuminance += pointlights[0].intensity * att * lambertian;

    for (int i = 0; i < pointlights.length(); i++)
	{
        PointLight light = pointlights[i];
		vec3 light_dir = normalize(light.pos - frag_pos_world);
        float lambertian = max(dot(light_dir, frag_normal), 0.0);
        float light_distance = distance(light.pos, frag_pos_world);
        float att = clamp(1.0 - light_distance * light_distance / (light.radius * light.radius), 0.0, 1.0);
        illuminance += light.intensity* att * lambertian;
	}

    // for (int i = 0; i < pointlights.length(); i++)
	// {
	// 	vec3 light_dir = normalize(pointlights[i].pos - frag_pos_world);
    //     float light_distance = distance(pointlights[i].pos, frag_pos_world);
    //     float att = clamp(1.0 - light_distance * light_distance / (pointlights[i].radius * pointlights[i].radius), 0.0, 1.0);
    //     illuminance += pointlights[i].intensity * att * lambertian;
	// }


    out_color = vec4(illuminance * texcolor, 1.0);
}
