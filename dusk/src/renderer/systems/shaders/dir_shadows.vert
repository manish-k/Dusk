#version 450

#extension GL_EXT_multiview : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 fragWorldPos;

layout (set = 0, binding = 0) buffer ModelMatrixBuffer 
{
	mat4 modelMatrix[];
};

layout(set = 1, binding = 1) buffer DirectionalLight
{
	int id;
	int pad0;
	int pad1;
	int pad2;
	mat4 projView;
	vec4 color;
	vec3 direction;
} dirLights[];

void main() 
{
	uint dirLightIdx = nonuniformEXT(gl_ViewIndex);

	mat4 modelMat = modelMatrix[gl_InstanceIndex];
	mat4 projViewMat = dirLights[dirLightIdx].projView;

	gl_Position = projViewMat * (modelMat * vec4(position, 1.0));
}