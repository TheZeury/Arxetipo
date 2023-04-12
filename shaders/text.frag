#version 450

layout (location = 0) in vec3 fragPosition;
layout (location = 1) in vec2 fragTextureUv;
layout (location = 2) in vec2 fragBitmapUv;
layout (location = 3) in vec3 fragNormal;
layout (location = 4) in mat3 TBN;

layout (set = 0, binding = 0) uniform MaterialPropertyBuffer {
    bool enableDiffuseMap;
    bool enableNormalMap;
    vec4 defaultColor;
} materialProperties;
layout (set = 0, binding = 1) uniform sampler2D diffuseSampler;
layout (set = 0, binding = 2) uniform sampler2D normalSampler;

layout (set = 1, binding = 0) uniform sampler2D bitmap;

layout (location = 0) out vec4 outColor;

vec3 lightDirection = { 1.0, 1.0, -0.4 };

vec4 lightColor = { 1.0, 1.0, 1.0, 1.0 };
float ambientValue = 0.4;
float diffuseValue;

void main() {
    float charOpaque = texture(bitmap, fragBitmapUv).r;
    if(charOpaque == 0.0)
    {
        discard;
    }
    vec3 normal = normalize(materialProperties.enableNormalMap ? (TBN * (texture(normalSampler, fragTextureUv).rgb * 2.0 - 1.0)) : fragNormal);
    vec4 ambientLight = lightColor * ambientValue;
    diffuseValue = max(dot(lightDirection, normalize(normal)), 0.0);
    vec4 diffuseLight = lightColor * diffuseValue;
    outColor = (materialProperties.enableDiffuseMap ? texture(diffuseSampler, fragTextureUv) : materialProperties.defaultColor) * (ambientLight + diffuseLight) * vec4(1.0, 1.0, 1.0, charOpaque);
    if(outColor.w < 0.3)
    {
        discard;
    }
}