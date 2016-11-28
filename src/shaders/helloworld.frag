#version 450
#extension GL_ARB_separate_shader_objects : enable

struct PointLight {
	vec3 pos;
	float radius;
	vec3 intensity;
};

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cam_pos;
} transform;

layout(set = 0, binding = 1) uniform sampler2D tex_sampler;
layout(set = 0, binding = 2) uniform sampler2D normal_sampler;
layout(set = 0, binding = 3) uniform PointLights // readonly buffer PointLights
{
	int light_num;
	PointLight pointlights[1000];
};

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_tex_coord;
layout(location = 2) in vec3 frag_normal;
layout(location = 3) in vec3 frag_pos_world;

layout(location = 0) out vec4 out_color;

const vec3 light_direction = vec3(1.0, 1.0, 1.0);

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
    vec4 color_map =  texture(tex_sampler, frag_tex_coord);
    vec3 diffuse = color_map.rgb;
    vec3 illuminance = vec3(0.0);

    vec3 normal = applyNormalMap(frag_normal, texture(normal_sampler, frag_tex_coord).rgb);

    for (int i = 0; i < light_num; i++)
	{
        PointLight light = pointlights[i];
		vec3 light_dir = normalize(light.pos - frag_pos_world);
        float lambertian = max(dot(light_dir, normal), 0.0);

        if(lambertian > 0.0)
        {
            float light_distance = distance(light.pos, frag_pos_world);
            if (light_distance > light.radius)
            {
                continue;
            }

            vec3 viewDir = normalize(transform.cam_pos - frag_pos_world);
            vec3 halfDir = normalize(light_dir + viewDir);
            float specAngle = max(dot(halfDir, normal), 0.0);
            float specular = pow(specAngle, 32.0); // TODO?: spec color & power in g-buffer?
        
            float att = clamp(1.0 - light_distance * light_distance / (light.radius * light.radius), 0.0, 1.0);
            illuminance += light.intensity * att * (lambertian * diffuse + specular);
        }
	}

    out_color = vec4(illuminance, 1.0);
    //out_color = vec4(abs(normal), 1.0);
    //out_color = vec4(abs(texture(normal_sampler, frag_tex_coord).rgb), 1.0); // normal map debug view
}
