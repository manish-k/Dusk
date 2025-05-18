#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform GlobalUBO 
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;
		
	uint directionalLightsCount;
	uint pointLightsCount;     
	uint spotLightsCount;      
	uint padding;
	
	vec4 directionalLightIndices[32];
	vec4 pointLightIndices[32];
	vec4 spotLightIndices[32];
} globalubo[];

layout (set = 0, binding = 1) uniform sampler2D textures[];

layout (set = 1, binding = 0) buffer Material 
{
	int id;
	int albedoTexId;
	int normalTexId;
	int padding;
	vec4 albedoColor;

} materials[];

layout(set = 2, binding = 1) uniform DirectionalLight
{
	int id;
	vec4 color;
	vec3 direction;
} dirLights[];

layout(push_constant) uniform DrawData 
{
	uint cameraIdx;
	uint materialIdx;
} push;

void main() {
	uint materialIdx = nonuniformEXT(push.materialIdx);
	int textureIdx = materials[materialIdx].albedoTexId;
	vec4 baseColor = materials[materialIdx].albedoColor;
	
	uint guboIdx = nonuniformEXT(push.cameraIdx);

	uint dirLightsCount = globalubo[guboIdx].directionalLightsCount;
	vec3 dirLightDir = dirLights[0].direction;
	
	outColor = baseColor * texture(textures[nonuniformEXT(textureIdx)], fragUV);
}