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
	
	uvec4 directionalLightIndices[32];
	uvec4 pointLightIndices[32];
	uvec4 spotLightIndices[32];
} globalubo[];

layout (set = 0, binding = 1) uniform sampler2D textures[];

layout (set = 1, binding = 0) buffer Material 
{
	int id;
	int albedoTexId;
	int normalTexId;
	int pad0;
	vec4 albedoColor;

} materials[];

layout(set = 2, binding = 1) buffer DirectionalLight
{
	int id;
	int pad0;
	int pad1;
	int pad2;
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

	vec4 dirColor = vec4(1.f, 1.f, 1.f, 1.f);
	uint dirCount  = globalubo[guboIdx].directionalLightsCount;
    uint vec4Count = (dirCount + 3u) >> 2;   // divide by 4, round up

    for (uint v = 0u; v < vec4Count; ++v)
    {
        uvec4 idx4 = globalubo[guboIdx].directionalLightIndices[v];

        if (4u*v + 0u < dirCount) dirColor = dirColor * dirLights[nonuniformEXT(idx4.x)].color;
        if (4u*v + 1u < dirCount) dirColor = dirColor * dirLights[nonuniformEXT(idx4.y)].color;
        if (4u*v + 2u < dirCount) dirColor = dirColor * dirLights[nonuniformEXT(idx4.z)].color;
        if (4u*v + 3u < dirCount) dirColor = dirColor * dirLights[nonuniformEXT(idx4.w)].color;
    }
	
	outColor = dirColor;
	//outColor = baseColor * texture(textures[nonuniformEXT(textureIdx)], fragUV);
}