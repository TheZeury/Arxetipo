#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

layout (location = 0) out vec3 fragPosition;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec2 fragUv;
layout (location = 3) out mat3 TBN;

layout (push_constant) uniform Push {
    mat4 projectionView;
    mat4 modelMatrix;
} push;

void main() {
    gl_Position = push.projectionView * vec4(position, 1.0);
    fragPosition = (push.modelMatrix * vec4(position, 1.0)).xyz;
    fragNormal = (push.modelMatrix * vec4(normal, 0.0)).xyz;
    fragUv = uv;

    vec3 T = normalize(vec3(push.modelMatrix * vec4(tangent, 0.0)));
    vec3 B = normalize(vec3(push.modelMatrix * vec4(bitangent, 0.0)));
    vec3 N = normalize(vec3(push.modelMatrix * vec4(normal,    0.0)));
    TBN = mat3(T, B, N);
}
