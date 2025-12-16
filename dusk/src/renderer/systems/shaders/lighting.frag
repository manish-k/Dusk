#version 450
#extension GL_ARB_shading_language_include : enable
#extension GL_EXT_nonuniform_qualifier : enable

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
		
	vec4 frustumPlanes[6];

	uint directionalLightsCount;
	uint pointLightsCount;     
	uint spotLightsCount;      
	uint padding;
	
	uvec4 directionalLightIndices[32];
	uvec4 pointLightIndices[32];
	uvec4 spotLightIndices[32];
} globalubo[];

layout (set = 1, binding = 0) uniform sampler2D textures[];
layout (set = 1, binding = 0) uniform samplerCube cubeTextures[];

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
	mat4 projView;
	vec4 color;
	vec3 direction;
} dirLights;

layout(set = 2, binding = 2) buffer PointLight
{
	int id;
	float constantAttenuationFactor;
	float linearAttenuationFactor;
	float quadraticAttenuationFactor;
	mat4 projView;
	vec4 color;
	vec3 position;
} pointLights[];

layout(set = 2, binding = 3) buffer SpotLight
{
	int id;
	float constantAttenuationFactor;
	float linearAttenuationFactor;
	float quadraticAttenuationFactor;
	mat4 projView;
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
	int aoRoughMetalTextureIdx;
	int emissiveTextureIdx;
	int depthTextureIdx;
	int irradianceTextureIdx;
	int prefilteredTextureIdx;
	int maxPrefilteredLODs;
    int brdfLUTIdx;
	int dirShadowMapIdx;
} push;

vec2 directionToEquirectangular(vec3 dir) {
    vec2 uv;
    uv.x = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    uv.y = asin(dir.y) / PI + 0.5;
    return uv;
}

vec3 computeDirLightsNonPBR(vec3 viewDirection, vec3 normal)
{
    vec3 lightDirection = normalize(-dirLights.direction);
	vec3 lightColor = dirLights.color.xyz;
	float lightIntensity = dirLights.color.w;
    
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
	vec3 fragWorldPos,
	vec3 albedo, 
	vec3 f0, 
	vec3 aoRM, 
	vec3 viewDirection, 
	vec3 normal)
{
	vec3 lightDirection = normalize(-dirLights.direction);
	vec3 halfDirection = normalize(lightDirection + viewDirection);
	
	vec3 lightColor = dirLights.color.xyz * dirLights.color.w;
	vec3 radiance = lightColor;
	
	// cook-torrance
	float r = (aoRM.g + 1.0f);
    float k = (r*r) / 8.0f;

	float ndf = distributionGGX(normal, halfDirection, aoRM.g);
	float g = geometrySmith(normal, viewDirection, lightDirection, k);
	vec3 f = fresnelSchlick(max(dot(halfDirection, viewDirection), 0.0), f0);

	vec3 ks = f;
	vec3 kd = vec3(1.0) - ks;
	kd *= 1.0 - aoRM.b;

	float ndotl = max(dot(normal, lightDirection), 0.0);
	float ndotv = max(dot(normal, viewDirection), 0.0);

	vec3 numer = ndf * g * f;
	float denom = 4.0 * ndotv * ndotl + 0.0001;
	vec3 specular = numer / denom;

	lightColor = (kd * albedo / PI + specular) * radiance * ndotl;

	// shadow calculations
	int dirShadowMapIdx = nonuniformEXT(push.dirShadowMapIdx);

	vec4 fragLightSpacePos = (dirLights.projView * vec4(fragWorldPos, 1.0));
	float NdotL = max(dot(normal, lightDirection), 0.0);
	vec3 projCoords = fragLightSpacePos.xyz / fragLightSpacePos.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5; // only xy because z is in [0,1] after perspective divide
	float currentDepth = projCoords.z;
	float bias =  max(0.0001 * (1.0 - NdotL), 0.00005);

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(textures[dirShadowMapIdx], 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(textures[dirShadowMapIdx], projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;

	return (1.0f - shadow) * lightColor;
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
	float r = (aoRM.g + 1.0);
    float k = (r*r) / 8.0;
	float g = geometrySmith(normal, viewDirection, lightDirection, k);
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
		float r = (aoRM.g + 1.0);
		float k = (r*r) / 8.0;
		float g = geometrySmith(normal, viewDirection, lightDirection, k);
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
	int irradianceTexIdx = nonuniformEXT(push.irradianceTextureIdx);
	int prefilteredTexIdx = nonuniformEXT(push.prefilteredTextureIdx);
	int brdfLUTIdx = nonuniformEXT(push.brdfLUTIdx);
	int emissiveTexIdx = nonuniformEXT(push.emissiveTextureIdx);

	vec3 surfaceNormal = normalize(texture(textures[normalTexIdx], fragUV).xyz * 2.0 - 1.0);
	vec3 albedo = texture(textures[albedoTexIdx], fragUV).xyz;
	float ndcDepth = texture(textures[depthTexIdx], fragUV).x;
	vec3 cameraPos = globalubo[guboIdx].inverseView[3].xyz;
	vec3 worldPos = worldPosFromDepth(fragUV, ndcDepth, globalubo[guboIdx].inverseProjection, globalubo[guboIdx].inverseView);
	vec3 viewDirection = normalize(cameraPos - worldPos);
	vec3 reflectDirection = normalize(reflect(-viewDirection, surfaceNormal));
	
	vec3 ambientColor = ambientLight.color.xyz * ambientLight.color.w;
	
	vec3 aoRM = texture(textures[aoRMTexIdx], fragUV).rgb;
	float metallic = aoRM.b;
	float roughness = aoRM.g;
	float ao = aoRM.r;
	
	vec3 f0 = vec3(0.04); 
    f0 = mix(f0, albedo, metallic);
	
	// reflectance from direct light
	//vec3 lightColor = vec3(0.03) * ambientColor * albedo; // non IBL ambience
	vec3 lightColor = vec3(0.0); 

	// compute contribution of all directional light
	uint dirCount  = globalubo[guboIdx].directionalLightsCount;
	if (dirCount > 0u)
	{
		lightColor += computeDirectionalLight(worldPos, albedo, f0, aoRM, viewDirection, surfaceNormal);
	}

	// compute contribution of all point lights
	uint pointCount  = globalubo[guboIdx].pointLightsCount;
    uint vec4Count = (pointCount + 3u) >> 2;   // divide by 4, round up
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

	// IBL ambient lighting
	float NdotV = max(dot(surfaceNormal, viewDirection), 0.0);
	vec3 f = fresnelSchlickRoughness(NdotV, f0, roughness);
	//vec3 f = fresnelSchlick(NdotV, f0);
    
    vec3 kS = f;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	

	// IBL diffuse
	vec3 irradiance = texture(cubeTextures[irradianceTexIdx], surfaceNormal).rgb;
	vec3 diffuse = irradiance * albedo;

	// IBL specular
	vec3 prefilteredColor = textureLod(cubeTextures[prefilteredTexIdx], reflectDirection, roughness * (push.maxPrefilteredLODs - 1)).rgb;
	vec2 brdf = texture(textures[brdfLUTIdx], vec2(NdotV, roughness)).xy;
	
	vec3 specular = prefilteredColor * (f * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * ao;

	vec3 finalColor = ambient + lightColor;

	// emissive color
	if (emissiveTexIdx > 0)
	{
		finalColor += texture(textures[emissiveTexIdx], fragUV).rgb;
	}

	// tone mapping
	finalColor = finalColor / (finalColor + vec3(1.0));

	outColor = vec4(finalColor.rgb, 1.0);
}