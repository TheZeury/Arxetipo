#version 450

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 color;

layout (location = 0) out vec2 fragUv;
layout (location = 1) out vec4 fragColor;

layout (push_constant) uniform Push {
    mat4 projectionView;
    mat4 modelMatrix;
} push;

void main()
{
	gl_Position =  push.projectionView * vec4(position, 0.0, 1.0);
    fragUv = uv;
    fragColor = color;
}