#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 1) in vec3 nearPoint;
layout(location = 2) in vec3 farPoint;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform GlobalUBO 
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;
	vec4 lightDirection;
	vec4 ambientLightColor;
} globalubo[];

layout(push_constant) uniform GridData 
{
	uint cameraBufferIdx;
} push;


void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
