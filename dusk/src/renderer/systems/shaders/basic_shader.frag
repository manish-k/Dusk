#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

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

vec3 computeDirectionalLight(uint lightIdx, vec3 viewDirection, vec3 normal)
{
    vec3 lightDir = normalize(-dirLights[lightIdx].direction);
	vec3 lightColor = dirLights[lightIdx].color.xyz;
	float lightIntensity = dirLights[lightIdx].color.w;
    
	// diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
	// specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDirection, reflectDir), 0.0), 300);
    
	// combine results
    vec3 diffuse  = lightColor * diff * lightIntensity;
    vec3 specular = lightColor * spec * lightIntensity;
    return (diffuse + specular);
}

void main() {
	uint guboIdx = nonuniformEXT(push.cameraIdx);

	vec3 surfaceNormal = normalize(fragNormal);
	vec3 cameraPos = globalubo[guboIdx].inverseView[3].xyz;
	vec3 viewDirection = normalize(cameraPos - fragWorldPos);

	uint materialIdx = nonuniformEXT(push.materialIdx);
	int textureIdx = materials[materialIdx].albedoTexId;
	vec4 baseColor = materials[materialIdx].albedoColor;
	
	vec3 dirColor = baseColor.xyz * texture(textures[nonuniformEXT(textureIdx)], fragUV).xyz;;
	uint dirCount  = globalubo[guboIdx].directionalLightsCount;
    uint vec4Count = (dirCount + 3u) >> 2;   // divide by 4, round up

    for (uint v = 0u; v < vec4Count; ++v)
    {
        uvec4 idx4 = globalubo[guboIdx].directionalLightIndices[v];

        if (4u*v + 0u < dirCount) dirColor = dirColor * computeDirectionalLight(idx4.x, viewDirection, surfaceNormal);
        if (4u*v + 1u < dirCount) dirColor = dirColor * computeDirectionalLight(idx4.y, viewDirection, surfaceNormal);
        if (4u*v + 2u < dirCount) dirColor = dirColor * computeDirectionalLight(idx4.z, viewDirection, surfaceNormal);
        if (4u*v + 3u < dirCount) dirColor = dirColor * computeDirectionalLight(idx4.w, viewDirection, surfaceNormal);
    }
	
	outColor = vec4(dirColor.xyz, 1.0f);
	//outColor = baseColor * texture(textures[nonuniformEXT(textureIdx)], fragUV);
}