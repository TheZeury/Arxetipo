#version 450

layout (location = 0) in vec3 frag_position;
layout (location = 1) in vec3 frag_normal;
layout (location = 2) in vec2 frag_uv;
layout (location = 3) in mat3 TBN;

layout (binding = 0) uniform MaterialPropertyBuffer {
    bool enable_diffuse_map;
    bool enable_normal_map;
    vec4 default_color;
} material_properties;
layout (binding = 1) uniform sampler2D diffuse_sampler;
layout (binding = 2) uniform sampler2D normal_sampler;

layout (location = 0) out vec4 out_color;

vec3 light_direction = { 1.0, 1.0, -0.4 };

vec3 light_color = { 1.0, 1.0, 1.0 };
float ambient_value = 0.4;
float diffuse_value;

void main() {
    vec3 normal = normalize(material_properties.enable_normal_map ? (TBN * (texture(normal_sampler, frag_uv).rgb * 2.0 - 1.0)) : frag_normal);
    vec3 ambientLight = light_color * ambient_value;
    diffuse_value = max(dot(light_direction, normalize(normal)), 0.0);
    vec3 diffuseLight = light_color * diffuse_value;
    out_color = (material_properties.enable_diffuse_map ? texture(diffuse_sampler, frag_uv) : material_properties.default_color);
    out_color.xyz *= (ambientLight + diffuseLight);
    if(out_color.a == 0.0)
    {
        discard;
    }
}
