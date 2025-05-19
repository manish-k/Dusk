#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragNormal;

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

layout (set = 3, binding = 0) uniform ModelUBO 
{
	mat4 modelMatrix;
	mat4 normalMatrix;
} modelubo;

layout(push_constant) uniform DrawData 
{
	uint cameraIdx;
	uint materialIdx;
} push;

void main() {	
	vec4 worldPos = modelubo.modelMatrix * vec4(position, 1.0);

	uint globalIdx = nonuniformEXT(push.cameraIdx);

	mat4 view = globalubo[globalIdx].view;
	mat4 proj = globalubo[globalIdx].projection;

	fragUV = uv;
	fragWorldPos = worldPos.xyz;
	fragNormal = normalize(mat3(modelubo.normalMatrix) * normal);
	
	gl_Position = proj * (view * worldPos);
}