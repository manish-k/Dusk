#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragUVW;

layout(location = 0) out vec4 outColor;

layout (set = 1, binding = 0) uniform samplerCube  textures[];

layout(push_constant) uniform SkyBoxPushConstant 
{
	uint frameIdx;
    int skyColorTextureIdx;
} push;


void main()
{
    int skyTexIdx = nonuniformEXT(push.skyColorTextureIdx);
    
    outColor    = texture(textures[skyTexIdx], fragUVW);
}
