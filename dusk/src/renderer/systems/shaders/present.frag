#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform PushConstant 
{
	uint inputTextureIdx;
} push;

void main() {
    uint inputTextureIdx = nonuniformEXT(push.inputTextureIdx);

    vec3 albedo = texture(textures[inputTextureIdx], fragUV).rgb;

    outColor = vec4(albedo, 1.0);
}