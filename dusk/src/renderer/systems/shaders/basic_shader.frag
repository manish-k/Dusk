#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform sampler2D textures[];

layout (set = 1, binding = 0) buffer Material 
{
	uint id;
	vec4 albedoColor;
	uint albedoTexId;
	uint normalTexId;
} materials[];

layout(push_constant) uniform DrawData 
{
	uint cameraIdx;
	uint materialIdx;
} push;

void main() {
	uint textureIdx = materials[nonuniformEXT(push.materialIdx)].albedoTexId;
	vec4 baseColor = materials[nonuniformEXT(push.materialIdx)].albedoColor;
	outColor = baseColor * texture(textures[nonuniformEXT(textureIdx)], fragUV);

	//outColor = vec4(0.7, 0.1, 0.1, 1.0);
}