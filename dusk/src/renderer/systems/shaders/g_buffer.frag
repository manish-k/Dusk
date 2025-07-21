#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;

layout (set = 0, binding = 0) uniform GlobalUBO 
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;
	mat4 inverseViewProjection;
		
	uint directionalLightsCount;
	uint pointLightsCount;     
	uint spotLightsCount;      
	uint padding;
	
	uvec4 directionalLightIndices[32];
	uvec4 pointLightIndices[32];
	uvec4 spotLightIndices[32];
} globalubo[];

layout (set = 1, binding = 0) buffer Material 
{
	int id;
	int albedoTexId;
	int normalTexId;
	int metallicRoughnessTexId;
	int aoTexId;          
	int emissiveTexId;    
	float aoStrength;       
	float emissiveIntensity;
	float normalScale;
	float metal;
	float rough;
	float padding0;
	vec4 albedoColor;
	vec4 emissiveColor;

} materials[];

layout (set = 3, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform DrawData 
{
	uint cameraIdx;
	uint materialIdx;
} push;

void main()
{
	uint guboIdx = nonuniformEXT(push.cameraIdx);

	vec3 surfaceNormal = normalize(fragNormal);
	vec3 cameraPos = globalubo[guboIdx].inverseView[3].xyz;
	vec3 viewDirection = normalize(cameraPos - fragWorldPos);

	uint materialIdx = nonuniformEXT(push.materialIdx);
	int textureIdx = materials[materialIdx].albedoTexId;
	vec4 baseColor = materials[materialIdx].albedoColor;

	vec3 lightColor = baseColor.xyz * texture(textures[nonuniformEXT(textureIdx)], fragUV).xyz;
	
	outColor = vec4(lightColor.xyz, 1.f);
	outNormal = vec4(surfaceNormal.xyz, 0.f);
}