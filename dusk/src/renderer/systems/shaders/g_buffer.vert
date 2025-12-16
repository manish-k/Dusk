#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragTangent;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out int fragInstanceId;

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

layout(push_constant) uniform DrawData 
{
	uint cameraIdx;
} push;

void main() {	
	uint globalIdx = nonuniformEXT(push.cameraIdx);

	mat4 model = meshInstanceData[gl_InstanceIndex].modelMatrix;
	mat4 view = globalubo[globalIdx].view;
	mat4 proj = globalubo[globalIdx].projection;
	mat3 normalMat = mat3(meshInstanceData[gl_InstanceIndex].normalMatrix);

	vec4 worldPos = model * vec4(position, 1.0);
	
	fragTangent = normalize(normalMat * tangent);
	fragNormal = normalize(normalMat * normal);

	fragUV = uv;
	fragWorldPos = worldPos.xyz;

	fragInstanceId = gl_InstanceIndex;

	gl_Position = proj * (view * worldPos);
}