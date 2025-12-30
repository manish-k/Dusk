#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;

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

layout (set = 2, binding = 0) buffer InstanceModelMatrixBuffer 
{
	mat4 modelMatrices[];
};

layout (set = 2, binding = 1) buffer InstanceNormalMatrixBuffer 
{
	mat4 normalMatrices[];
};

layout(push_constant) uniform DrawData 
{
	uint cameraIdx;
} push;

void main() {	
	uint globalIdx = nonuniformEXT(push.cameraIdx);

	mat4 model = modelMatrices[gl_InstanceIndex];
	mat4 view = globalubo[globalIdx].view;
	mat4 proj = globalubo[globalIdx].projection;
	mat3 normalMat = mat3(normalMatrices[gl_InstanceIndex]);

	vec4 worldPos = model * vec4(position, 1.0);
	
	fragTangent = normalize(normalMat * tangent);
	fragNormal = normalize(normalMat * normal);

	fragUV = uv;
	fragWorldPos = worldPos.xyz;

	fragInstanceId = gl_InstanceIndex;

	gl_Position = proj * (view * worldPos);
}