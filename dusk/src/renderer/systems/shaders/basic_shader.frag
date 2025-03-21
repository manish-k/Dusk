#version 450

layout(location = 0) in vec3 fragColor;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;
	vec4 lightDirection;
	vec4 ambientLightColor;
} ubo;

layout(push_constant) uniform Push 
{
	mat4 model;
	mat4 normal;
} push;

void main() {	
	outColor = vec4(fragColor, 1.0f);
}