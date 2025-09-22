#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_include : enable

#include "common.glsl"
#include "brdf.glsl"

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform GlobalUBO 
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;
	mat4 inverseProjection;
		
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
	int albedoTextureIdx;
    int normalTextureIdx;
    int depthTextureIdx;
	int aoRoughMetalTextureIdx;
} push;

vec3 computeDirLightsPBR(uint lightIdx, vec3 viewDirection, vec3 normal)
{
    vec3 lightDirection = normalize(-dirLights[lightIdx].direction);
	vec3 lightColor = dirLights[lightIdx].color.xyz;
	float lightIntensity = dirLights[lightIdx].color.w;
    
	// diffuse shading
    float diff = max(dot(normal, lightDirection), 0.0);
    
	// specular shading
    //vec3 reflectDirection = reflect(-lightDirection, normal);
    //float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), 300);
	vec3 halfAngle = normalize(lightDirection + viewDirection);
	float blinnTerm = dot(normal, halfAngle);
	blinnTerm = clamp(blinnTerm, 0, 1);
	float spec = pow(blinnTerm, 512.0);
    
	// combine results
    vec3 diffuse  = lightColor * diff * lightIntensity;
    vec3 specular = lightColor * spec * lightIntensity;
    return (diffuse + specular);
}

vec3 computeDirectionalLight(
	uint lightIdx, 
	vec3 albedo, 
	vec3 f0, 
	vec3 aoRM, 
	vec3 viewDirection, 
	vec3 normal)
{
	vec3 lightDirection = normalize(-dirLights[lightIdx].direction);
	vec3 halfDirection = normalize(lightDirection + viewDirection);
	
	vec3 lightColor = dirLights[lightIdx].color.xyz * dirLights[lightIdx].color.w;
	vec3 radiance = lightColor;
	
	// cook-torrance
	float ndf = distributionGGX(normal, halfDirection, aoRM.g);
	float g = geometrySmith(normal, viewDirection, halfDirection, aoRM.g);
	vec3 f = fresnelSchlick(max(dot(halfDirection, viewDirection), 0.0), f0);

	vec3 ks = f;
	vec3 kd = vec3(1.0) - ks;
	kd *= 1.0 - aoRM.b;

	float ndotl = max(dot(normal, lightDirection), 0.0);
	float ndotv = max(dot(normal, viewDirection), 0.0);

	vec3 numer = ndf * g * f;
	float denom = 4.0 * ndotv * ndotl + 0.0001;
	vec3 specular = numer / denom;

	return (kd * albedo / PI + specular) * radiance * ndotl;
}

vec3 computePointLight(
	uint lightIdx,
	vec3 albedo, 
	vec3 f0, 
	vec3 aoRM, 
	vec3 fragPosition, 
	vec3 viewDirection, 
	vec3 normal)
{
	vec3 lightPosition = pointLights[lightIdx].position;
	vec3 lightDirection = normalize(lightPosition - fragPosition);
	vec3 halfDirection = normalize(lightDirection + viewDirection);

	vec3 lightColor = pointLights[lightIdx].color.xyz * pointLights[lightIdx].color.w;
    
	// attenuation
	float constant = pointLights[lightIdx].constantAttenuationFactor;
	float linear = pointLights[lightIdx].linearAttenuationFactor;
	float quad = pointLights[lightIdx].quadraticAttenuationFactor;
    float distance    = length(lightPosition - fragPosition);
    float attenuation = 1.0 / (constant + linear * distance + 
  			     quad * (distance * distance));

	vec3 radiance = lightColor * attenuation;
    
	// cook-torrance
	float ndf = distributionGGX(normal, halfDirection, aoRM.g);
	float g = geometrySmith(normal, viewDirection, halfDirection, aoRM.g);
	vec3 f = fresnelSchlick(max(dot(halfDirection, viewDirection), 0.0), f0);

	vec3 ks = f;
	vec3 kd = vec3(1.0) - ks;
	kd *= 1.0 - aoRM.b;

	float ndotl = max(dot(normal, lightDirection), 0.0);
	float ndotv = max(dot(normal, viewDirection), 0.0);

	vec3 numer = ndf * g * f;
	float denom = 4.0 * ndotv * ndotl + 0.0001;
	vec3 specular = numer / denom;

	return (kd * albedo / PI + specular) * radiance * ndotl;
}

vec3 computeSpotLight(
	uint lightIdx,
	vec3 albedo, 
	vec3 f0, 
	vec3 aoRM, 
	vec3 fragPosition, 
	vec3 viewDirection, 
	vec3 normal)
{
	vec3 lightPosition = spotLights[lightIdx].position;
	vec3 lightSrcDirection = normalize(lightPosition - fragPosition);
	vec3 lightDirection = normalize(-spotLights[lightIdx].direction);
	vec3 halfDirection = normalize(lightDirection + viewDirection);

	vec3 lightColor = spotLights[lightIdx].color.xyz * spotLights[lightIdx].color.w;

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

		lightColor *= intensityFalloffMult;
		
		// attenuation
		float constant = pointLights[lightIdx].constantAttenuationFactor;
		float linear = pointLights[lightIdx].linearAttenuationFactor;
		float quad = pointLights[lightIdx].quadraticAttenuationFactor;
		float distance    = length(lightPosition - fragPosition);
		float attenuation = 1.0 / (constant + linear * distance + 
  					 quad * (distance * distance));

		vec3 radiance = lightColor * attenuation;
    
		// cook-torrance
		float ndf = distributionGGX(normal, halfDirection, aoRM.g);
		float g = geometrySmith(normal, viewDirection, halfDirection, aoRM.g);
		vec3 f = fresnelSchlick(max(dot(halfDirection, viewDirection), 0.0), f0);

		vec3 ks = f;
		vec3 kd = vec3(1.0) - ks;
		kd *= 1.0 - aoRM.b;

		float ndotl = max(dot(normal, lightDirection), 0.0);
		float ndotv = max(dot(normal, viewDirection), 0.0);

		vec3 numer = ndf * g * f;
		float denom = 4.0 * ndotv * ndotl + 0.0001;
		vec3 specular = numer / denom;

		return (kd * albedo / PI + specular) * radiance * ndotl;
	}
	else
	{
		return vec3(0.f); // no color
	}
}

void main() {
    uint guboIdx = nonuniformEXT(push.frameIdx);
	int albedoTexIdx = nonuniformEXT(push.albedoTextureIdx);
	int normalTexIdx = nonuniformEXT(push.normalTextureIdx);
	int depthTexIdx = nonuniformEXT(push.depthTextureIdx);
	int aoRMTexIdx = nonuniformEXT(push.aoRoughMetalTextureIdx);

	vec3 surfaceNormal = normalize(texture(textures[normalTexIdx], fragUV).xyz * 2.0 - 1.0);
	vec3 albedo = texture(textures[albedoTexIdx], fragUV).xyz;
	float ndcDepth = texture(textures[depthTexIdx], fragUV).x;
	vec3 cameraPos = globalubo[guboIdx].inverseView[3].xyz;
	vec3 worldPos = worldPosFromDepth(fragUV, ndcDepth, globalubo[guboIdx].inverseProjection, globalubo[guboIdx].inverseView);
	vec3 viewDirection = normalize(cameraPos - worldPos);
	
	vec3 ambientColor = ambientLight.color.xyz * ambientLight.color.w;
	
	vec3 aoRM = texture(textures[aoRMTexIdx], fragUV).rgb;
	vec3 f0 = vec3(0.04); 
    f0 = mix(f0, albedo, aoRM.b);
	
	// adding ambient
	vec3 lightColor = vec3(0.03) * ambientColor * albedo;

	// compute contribution of all directional light
	uint dirCount  = globalubo[guboIdx].directionalLightsCount;
    uint vec4Count = (dirCount + 3u) >> 2;   // divide by 4, round up
    for (uint v = 0u; v < vec4Count; ++v)
    {
        uvec4 idx4 = globalubo[guboIdx].directionalLightIndices[v];

        if (4u*v + 0u < dirCount) lightColor += computeDirectionalLight(idx4.x, albedo, f0, aoRM, viewDirection, surfaceNormal);
        if (4u*v + 1u < dirCount) lightColor += computeDirectionalLight(idx4.y, albedo, f0, aoRM, viewDirection, surfaceNormal);
        if (4u*v + 2u < dirCount) lightColor += computeDirectionalLight(idx4.z, albedo, f0, aoRM, viewDirection, surfaceNormal);
        if (4u*v + 3u < dirCount) lightColor += computeDirectionalLight(idx4.w, albedo, f0, aoRM, viewDirection, surfaceNormal);
    }

	// compute contribution of all point lights
	uint pointCount  = globalubo[guboIdx].pointLightsCount;
    vec4Count = (pointCount + 3u) >> 2;   // divide by 4, round up
    for (uint v = 0u; v < vec4Count; ++v)
    {
        uvec4 idx4 = globalubo[guboIdx].pointLightIndices[v];

        if (4u*v + 0u < pointCount) lightColor = lightColor + computePointLight(idx4.x, albedo, f0, aoRM, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 1u < pointCount) lightColor = lightColor + computePointLight(idx4.y, albedo, f0, aoRM, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 2u < pointCount) lightColor = lightColor + computePointLight(idx4.z, albedo, f0, aoRM, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 3u < pointCount) lightColor = lightColor + computePointLight(idx4.w, albedo, f0, aoRM, worldPos, viewDirection, surfaceNormal);
    }

	// compute contribution of all spot lights
	uint spotCount  = globalubo[guboIdx].spotLightsCount;
    vec4Count = (spotCount + 3u) >> 2;   // divide by 4, round up
    for (uint v = 0u; v < vec4Count; ++v)
    {
        uvec4 idx4 = globalubo[guboIdx].spotLightIndices[v];

        if (4u*v + 0u < spotCount) lightColor = lightColor + computeSpotLight(idx4.x, albedo, f0, aoRM, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 1u < spotCount) lightColor = lightColor + computeSpotLight(idx4.y, albedo, f0, aoRM, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 2u < spotCount) lightColor = lightColor + computeSpotLight(idx4.z, albedo, f0, aoRM, worldPos, viewDirection, surfaceNormal);
        if (4u*v + 3u < spotCount) lightColor = lightColor + computeSpotLight(idx4.w, albedo, f0, aoRM, worldPos, viewDirection, surfaceNormal);
    }

	// tone mapping
	lightColor = lightColor / (lightColor + vec3(1.0));
   
	outColor = vec4(lightColor.xyz, 1.0f);
}