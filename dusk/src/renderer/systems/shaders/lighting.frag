#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_include : enable

#line 0
#include "common.glsl"

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

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

layout (set = 1, binding = 0) uniform sampler2D textures[];

layout(set = 2, binding = 0) buffer AmbientLight
{
	vec4 color;
} ambientLight;

layout(set = 2, binding = 1) buffer DirectionalLight
{
	int id;
	int pad0;
	int pad1;
	int pad2;
	vec4 color;
	vec3 direction;
} dirLights[];

layout(set = 2, binding = 2) buffer PointLight
{
	int id;
	float constantAttenuationFactor;
	float linearAttenuationFactor;
	float quadraticAttenuationFactor;
	vec4 color;
	vec3 position;
} pointLights[];

layout(set = 2, binding = 3) buffer SpotLight
{
	int id;
	float constantAttenuationFactor;
	float linearAttenuationFactor;
	float quadraticAttenuationFactor;
	vec4 color;
	vec3 position;
	float innerCutOff;
	vec3 direction;
	float outerCutOff;
} spotLights[];

layout(push_constant) uniform PushConstant 
{
	uint frameIdx;
	uint albedoTextureIdx;
    uint normalTextureIdx;
    uint depthTextureIdx;
} push;

vec3 computeDirectionalLight(uint lightIdx, vec3 viewDirection, vec3 normal)
{
    vec3 lightDirection = normalize(-dirLights[lightIdx].direction);
	vec3 lightColor = dirLights[lightIdx].color.xyz;
	float lightIntensity = dirLights[lightIdx].color.w;
    
	// diffuse shading
    float diff = max(dot(normal, lightDirection), 0.0);
    
	// specular shading
    vec3 reflectDirection = reflect(-lightDirection, normal);
    float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), 300);
    
	// combine results
    vec3 diffuse  = lightColor * diff * lightIntensity;
    vec3 specular = lightColor * spec * lightIntensity;
    return (diffuse + specular);
}

vec3 computePointLight(uint lightIdx, vec3 fragPosition, vec3 viewDirection, vec3 normal)
{
	vec3 lightPosition = pointLights[lightIdx].position;
	vec3 lightDirection = normalize(lightPosition - fragPosition);
	vec3 lightColor = pointLights[lightIdx].color.xyz;
	float lightIntensity = pointLights[lightIdx].color.w;
    
	// diffuse shading
    float diff = max(dot(normal, lightDirection), 0.0);
    
	// specular shading
    vec3 reflectDirection = reflect(-lightDirection, normal);
    float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), 300);
    
	// attenuation
	float constant = pointLights[lightIdx].constantAttenuationFactor;
	float linear = pointLights[lightIdx].linearAttenuationFactor;
	float quad = pointLights[lightIdx].quadraticAttenuationFactor;
    float distance    = length(lightPosition - fragPosition);
    float attenuation = 1.0 / (constant + linear * distance + 
  			     quad * (distance * distance));    
    
	// combine results
    vec3 diffuse  = lightColor * diff * lightIntensity;
    vec3 specular = lightColor * spec * lightIntensity;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (diffuse + specular);
}

vec3 computeSpotLight(uint lightIdx, vec3 fragPosition, vec3 viewDirection, vec3 normal)
{
	vec3 lightPosition = spotLights[lightIdx].position;
	vec3 lightSrcDirection = normalize(lightPosition - fragPosition);
	vec3 lightDirection = normalize(-spotLights[lightIdx].direction);
	vec3 lightColor = spotLights[lightIdx].color.xyz;
	float lightIntensity = spotLights[lightIdx].color.w;

	// cutoff calculations
	float theta     = dot(lightSrcDirection, lightDirection);
	float innerCutOff = spotLights[lightIdx].innerCutOff;
	float outerCutOff = spotLights[lightIdx].outerCutOff;
	float epsilon   = innerCutOff - outerCutOff;
	
	if (theta > outerCutOff)
	{
		float intensityFalloffMult = 1.f;
		if (theta < innerCutOff)
			intensityFalloffMult = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

		// diffuse shading
		float diff = max(dot(normal, lightSrcDirection), 0.0);
    
		// specular shading
		vec3 reflectDirection = reflect(-lightSrcDirection, normal);
		float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), 300);
    
		// attenuation
		float constant = spotLights[lightIdx].constantAttenuationFactor;
		float linear = spotLights[lightIdx].linearAttenuationFactor;
		float quad = spotLights[lightIdx].quadraticAttenuationFactor;
		float distance    = length(lightPosition - fragPosition);
		float attenuation = 1.0 / (constant + linear * distance + 
  					 quad * (distance * distance));    
    
		// combine results
		vec3 diffuse  = lightColor * diff * lightIntensity * intensityFalloffMult;
		vec3 specular = lightColor * spec * lightIntensity * intensityFalloffMult;
		diffuse  *= attenuation;
		specular *= attenuation;
		return (diffuse + specular);
	}
	else
	{
		return vec3(0.f); // no color
	}
}


void main() {
    uint guboIdx = nonuniformEXT(push.frameIdx);
	uint albedoTexIdx = nonuniformEXT(push.albedoTextureIdx);
	uint normalTexIdx = nonuniformEXT(push.normalTextureIdx);
	uint depthTexIdx = nonuniformEXT(push.depthTextureIdx);

	vec3 surfaceNormal = normalize(texture(textures[normalTexIdx], fragUV).xyz);
	vec3 baseColor = texture(textures[albedoTexIdx], fragUV).xyz;
	float ndcDepth = texture(textures[depthTexIdx], fragUV).x;
	
	
	vec3 cameraPos = globalubo[guboIdx].inverseView[3].xyz;
	vec3 worldPos = worldPosFromDepth(fragUV, ndcDepth, globalubo[guboIdx].inverseViewProjection);
	vec3 viewDirection = normalize(cameraPos - worldPos);
	
	vec3 ambientColor = ambientLight.color.xyz * ambientLight.color.w;

	vec3 lightColor = ambientColor * baseColor.xyz;
	
	// compute contribution of all directional light
	uint dirCount  = globalubo[guboIdx].directionalLightsCount;
    uint vec4Count = (dirCount + 3u) >> 2;   // divide by 4, round up
    for (uint v = 0u; v < vec4Count; ++v)
    {
        uvec4 idx4 = globalubo[guboIdx].directionalLightIndices[v];

        if (4u*v + 0u < dirCount) lightColor = lightColor + computeDirectionalLight(idx4.x, viewDirection, surfaceNormal);
        if (4u*v + 1u < dirCount) lightColor = lightColor + computeDirectionalLight(idx4.y, viewDirection, surfaceNormal);
        if (4u*v + 2u < dirCount) lightColor = lightColor + computeDirectionalLight(idx4.z, viewDirection, surfaceNormal);
        if (4u*v + 3u < dirCount) lightColor = lightColor + computeDirectionalLight(idx4.w, viewDirection, surfaceNormal);
    }

	// compute contribution of all point lights
	uint pointCount  = globalubo[guboIdx].pointLightsCount;
    vec4Count = (pointCount + 3u) >> 2;   // divide by 4, round up
    for (uint v = 0u; v < vec4Count; ++v)
    {
        uvec4 idx4 = globalubo[guboIdx].pointLightIndices[v];

        if (4u*v + 0u < pointCount) lightColor = lightColor + computePointLight(idx4.x, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 1u < pointCount) lightColor = lightColor + computePointLight(idx4.y, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 2u < pointCount) lightColor = lightColor + computePointLight(idx4.z, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 3u < pointCount) lightColor = lightColor + computePointLight(idx4.w, worldPos, viewDirection, surfaceNormal);
    }

	// compute contribution of all spot lights
	uint spotCount  = globalubo[guboIdx].spotLightsCount;
    vec4Count = (spotCount + 3u) >> 2;   // divide by 4, round up
    for (uint v = 0u; v < vec4Count; ++v)
    {
        uvec4 idx4 = globalubo[guboIdx].spotLightIndices[v];

        if (4u*v + 0u < spotCount) lightColor = lightColor + computeSpotLight(idx4.x, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 1u < spotCount) lightColor = lightColor + computeSpotLight(idx4.y, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 2u < spotCount) lightColor = lightColor + computeSpotLight(idx4.z, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 3u < spotCount) lightColor = lightColor + computeSpotLight(idx4.w, worldPos, viewDirection, surfaceNormal);
    }

	outColor = vec4(lightColor.xyz, 1.0f);
}