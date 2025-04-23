#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

layout (set = 0, binding = 0) uniform GlobalUBO 
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;
	vec4 lightDirection;
	vec4 ambientLightColor;
} globalubo[];

layout (set = 2, binding = 0) uniform ModelUBO 
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

	mat4 view = globalubo[nonuniformEXT(push.cameraIdx)].view;
	mat4 proj = globalubo[nonuniformEXT(push.cameraIdx)].projection;

	fragUV = uv;
	gl_Position = proj * (view * worldPos);
}