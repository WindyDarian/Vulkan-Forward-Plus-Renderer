#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, set = 0, binding = 0) uniform SceneObjectUbo
{
    mat4 model;
} transform;

layout(std140, set = 1, binding = 0) buffer readonly CameraUbo // FIXME: change back to uniform
{
    mat4 view;
    mat4 proj;
    mat4 projview;
    vec3 cam_pos;
} camera;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex_coord;
layout(location = 2) out vec3 frag_normal;
layout(location = 3) out vec3 frag_pos_world;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    // TODO: calculate them upfront, in CPU or something
    mat4 invtransmodel =  transpose(inverse(transform.model));
    mat4 mvp = camera.projview * transform.model;

    gl_Position = mvp * vec4(in_position, 1.0);
    frag_color = in_color;
    frag_tex_coord = in_tex_coord;

    // TODO: do everything view or projection space
    frag_normal = normalize((invtransmodel * vec4(in_normal, 0.0)).xyz);
    frag_pos_world = vec3(transform.model * vec4(in_position, 1.0));
}
