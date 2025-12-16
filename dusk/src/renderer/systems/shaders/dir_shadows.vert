#version 450

#extension GL_EXT_multiview : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 fragWorldPos;

layout (set = 0, binding = 0) uniform GlobalUBO 
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;

	vec4 frustumPlanes[6];
		
	uint directionalLightsCount;
	uint pointLightsCount;     
	uint spotLightsCount;      
	uint padding;
	
	uvec4 directionalLightIndices[32];
	uvec4 pointLightIndices[32];
	uvec4 spotLightIndices[32];
} globalubo[];

layout (set = 1, binding = 0) buffer ModelUBO 
{
	mat4 modelMatrix;
	mat4 normalMatrix;
} modelubo[];

layout(set = 2, binding = 1) buffer DirectionalLight
{
	int id;
	int pad0;
	int pad1;
	int pad2;
	mat4 projView;
	vec4 color;
	vec3 direction;
} dirLights[];

layout(push_constant) uniform ShadowMapData 
{
	uint frameIdx;
	uint modelIdx;
} push;

void main() 
{
	uint globalIdx = nonuniformEXT(push.frameIdx);
	uint modelIdx = nonuniformEXT(push.modelIdx);
	uint dirLightIdx = nonuniformEXT(gl_ViewIndex);

	mat4 model = modelubo[modelIdx].modelMatrix;
	mat4 projView = dirLights[dirLightIdx].projView;

	gl_Position = projView * (model * vec4(position, 1.0));
}