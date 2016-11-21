#version 450
#extension GL_ARB_separate_shader_objects : enable

// layout(set = 0, binding = 0) uniform UniformBufferObject
// {
//     mat4 model;
//     mat4 view;
//     mat4 proj;
// } transform;

layout(binding = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_tex_coord;
layout(location = 2) in vec3 frag_normal;

layout(location = 0) out vec4 out_color;

const vec3 light_direction = vec3(1.0, 1.0, 1.0);

void main()
{
    vec4 color_map =  texture(tex_sampler, frag_tex_coord);
    vec3 color = color_map.rgb;
    out_color = vec4(abs(frag_normal), 1.0);
    float lambertian = max(dot(light_direction, frag_normal), 0.0);
    //out_color = texture(tex_sampler, frag_tex_coord) * 0.75 + vec4(frag_color, 1.0) * 0.3;
    //out_color = vec4(lambertian * color, 1.0);
}
