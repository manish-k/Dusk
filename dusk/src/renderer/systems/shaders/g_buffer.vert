#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;
layout(location = 5) in vec2 uv;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragTangent;
layout(location = 3) out vec3 fragNormal;



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

layout (set = 2, binding = 0) buffer ModelUBO 
{
	mat4 modelMatrix;
	mat4 normalMatrix;
} modelubo[];

layout(push_constant) uniform DrawData 
{
	uint cameraIdx;
	uint materialIdx;
	uint modelIdx;
} push;

void main() {	
	uint globalIdx = nonuniformEXT(push.cameraIdx);
	uint modelIdx = nonuniformEXT(push.modelIdx);

	mat4 model = modelubo[modelIdx].modelMatrix;
	mat4 view = globalubo[globalIdx].view;
	mat4 proj = globalubo[globalIdx].projection;
	mat3 normalMat = mat3(modelubo[modelIdx].normalMatrix);

	vec4 worldPos = model * vec4(position, 1.0);
	
	fragTangent = normalize(normalMat * tangent);
	fragNormal = normalize(normalMat * normal);

	fragUV = uv;
	fragWorldPos = worldPos.xyz;

	gl_Position = proj * (view * worldPos); 
}