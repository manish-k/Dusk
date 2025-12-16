#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragTangent;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in flat int fragInstanceId;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal; // TODO: explore octohedral representation for effecient packing
layout (location = 2) out vec4 outAORoughMetal; // R: AO, G: Roughness, B: Metallic
layout (location = 3) out vec4 outEmissiveColor;

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

layout (set = 2, binding = 0) buffer MeshInstanceData 
{
	mat4 modelMatrix;
	mat4 normalMatrix;
	vec3 aabbMin;
	uint pad0;
	vec3 aabbMax;
	uint pad1;
	uint materialId;
} meshInstanceData[];

layout (set = 3, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform DrawData 
{
	uint cameraIdx;
} push;

void main()
{
	uint guboIdx = nonuniformEXT(push.cameraIdx);
	uint materialIdx = nonuniformEXT(meshInstanceData[fragInstanceId].materialId);
	int textureIdx = nonuniformEXT(materials[materialIdx].albedoTexId);
	int normalTexIdx = nonuniformEXT(materials[materialIdx].normalTexId);
	int metalRoughTexIdx = nonuniformEXT(materials[materialIdx].metallicRoughnessTexId);
	int aoTexIdx = nonuniformEXT(materials[materialIdx].aoTexId);
	int emissiveTexIdx = nonuniformEXT(materials[materialIdx].emissiveTexId);
	
	vec3 cameraPos = globalubo[guboIdx].inverseView[3].xyz;
	vec3 viewDirection = normalize(cameraPos - fragWorldPos);

	vec4 baseColor = materials[materialIdx].albedoColor;
	vec3 lightColor = baseColor.xyz * texture(textures[textureIdx], fragUV).xyz;
	
	vec3 mrSampleValue = texture(textures[metalRoughTexIdx], fragUV).rgb;
	float ao = texture(textures[aoTexIdx], fragUV).r * materials[materialIdx].aoStrength;
	if (ao <= 0) ao = 1;

	float roughness = mrSampleValue.g * materials[materialIdx].rough;
	float metallic = mrSampleValue.b * materials[materialIdx].metal;

	vec3 surfaceNormal = normalize(fragNormal);
	if (normalTexIdx != -1)
	{
		vec3 tangent = normalize(fragTangent);
	
		// Gram-Schmidt orthogonalize
		tangent = normalize(tangent - dot(tangent, surfaceNormal) * surfaceNormal);
		vec3 bitangent = cross(surfaceNormal, tangent);

		mat3 TBN = mat3(tangent, bitangent, surfaceNormal);

		vec3 texNormal = texture(textures[normalTexIdx], fragUV).xyz * 2.0 - 1.0;
		surfaceNormal = normalize(TBN * normalize(texNormal));
	}

	vec3 emissiveColor = texture(textures[emissiveTexIdx], fragUV).rgb;

	outColor = vec4(lightColor.rgb, 1.f);
	outNormal = vec4(surfaceNormal.xyz * 0.5 + 0.5, 0.f);
	outAORoughMetal = vec4(ao, roughness, metallic, 1.0f);
	outEmissiveColor = vec4(emissiveColor.rgb, 1.0f);
}