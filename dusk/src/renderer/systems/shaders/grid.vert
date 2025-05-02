#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 1) out vec3 nearPoint;
layout(location = 2) out vec3 farPoint;

layout (set = 0, binding = 0) uniform GlobalUBO 
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;
	vec4 lightDirection;
	vec4 ambientLightColor;
} globalubo[];

layout(push_constant) uniform GridData 
{
	uint cameraBufferIdx;
} push;

vec3 gridPlane[6] = vec3[](
    vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(-1, -1, 0), vec3(1, 1, 0), vec3(1, -1, 0)
);

void main() {
	mat4 view = globalubo[nonuniformEXT(push.cameraBufferIdx)].view;
	mat4 inverseView = globalubo[nonuniformEXT(push.cameraBufferIdx)].inverseView;
	mat4 proj = globalubo[nonuniformEXT(push.cameraBufferIdx)].projection;
	mat4 inverseProj = inverse(proj);

	vec3 point = gridPlane[gl_VertexIndex];

	nearPoint = (inverseView * inverseProj * vec4(point.x, point.y, 0.0, 1)).xyz;
	farPoint = (inverseView * inverseProj * vec4(point.x, point.y, 1.0, 1)).xyz;

    gl_Position = vec4(point.xyz, 1.0);
}