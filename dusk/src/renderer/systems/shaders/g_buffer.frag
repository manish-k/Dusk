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

struct Material 
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

};

layout (set = 0, binding = 0) uniform GlobalUBO 
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;
	mat4 inverseViewProjection;

	vec4 frustumPlanes[6];
		
	uint directionalLightsCount;
	uint pointLightsCount;     
	uint spotLightsCount;      
	uint padding;
	
	uvec4 directionalLightIndices[32];
	uvec4 pointLightIndices[32];
	uvec4 spotLightIndices[32];
} globalubo[];

layout (set = 1, binding = 0) buffer MaterialBuffer 
{
	Material mat;

} materials[];

layout (set = 2, binding = 0) buffer MeshInstanceData 
{
	mat4 modelMatrix;
	mat4 normalMatrix;
	vec3 aabbMin;
	uint pad0;
	vec3 aabbMax;
	uint materialId;
	uint indexCount;   
	uint firstIndex;
	uint vertexOffset;
	uint firstInstance;
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
	
	// pull and cache
	Material m = materials[materialIdx].mat;
	
	int albedoTexIdx  = nonuniformEXT(m.albedoTexId);
	int normalTexIdx = nonuniformEXT(m.normalTexId);
	int metalRoughTexIdx = nonuniformEXT(m.metallicRoughnessTexId);
	int aoTexIdx = nonuniformEXT(m.aoTexId);
	int emissiveTexIdx = nonuniformEXT(m.emissiveTexId);

	vec4 albedoSample   = vec4(1.0);
    vec3 mrSample       = vec3(1.0);
    float aoSample      = 1.0;
    vec3 emissiveSample = vec3(0.0);
    vec3 normalSample   = vec3(0.0);

	// fetch textures
	if (albedoTexIdx >= 0)
        albedoSample = texture(textures[albedoTexIdx], fragUV);

    if (metalRoughTexIdx >= 0)
        mrSample = texture(textures[metalRoughTexIdx], fragUV).rgb;

    if (aoTexIdx >= 0)
        aoSample = texture(textures[aoTexIdx], fragUV).r;

    if (emissiveTexIdx >= 0)
        emissiveSample = texture(textures[emissiveTexIdx], fragUV).rgb;

    if (normalTexIdx >= 0)
        normalSample = texture(textures[normalTexIdx], fragUV).xyz * 2.0 - 1.0;

	vec3 emissiveColor = texture(textures[emissiveTexIdx], fragUV).rgb;
	
	vec3 cameraPos = globalubo[guboIdx].inverseView[3].xyz;
	vec3 viewDirection = normalize(cameraPos - fragWorldPos);

	vec4 baseColor = m.albedoColor;
	vec3 lightColor = baseColor.xyz * albedoSample.xyz;
	
	float ao = aoSample.r * m.aoStrength;
	if (ao <= 0) ao = 1;

	float roughness = mrSample.g * m.rough;
	float metallic = mrSample.b * m.metal;

	vec3 surfaceNormal = normalize(fragNormal);
	if (normalTexIdx >= 0)
	{
		vec3 tangent = normalize(fragTangent);
	
		// Gram-Schmidt orthogonalize
		tangent = normalize(tangent - dot(tangent, surfaceNormal) * surfaceNormal);
		vec3 bitangent = cross(surfaceNormal, tangent);

		mat3 TBN = mat3(tangent, bitangent, surfaceNormal);

		surfaceNormal = normalize(TBN * normalize(normalSample));
	}


	outColor = vec4(lightColor.rgb, 1.f);
	outNormal = vec4(surfaceNormal.xyz * 0.5 + 0.5, 0.f);
	outAORoughMetal = vec4(ao, roughness, metallic, 1.0f);
	outEmissiveColor = vec4(emissiveColor.rgb, 1.0f);
}