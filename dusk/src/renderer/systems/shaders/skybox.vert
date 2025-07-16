#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragUVW;

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

layout(push_constant) uniform SkyBoxPushConstant 
{
	uint frameIdx;
    uint skyColorTextureIdx;
} push;

void main()
{	
	fragUVW = position;
	fragUVW.xy *= -1.0f;

	mat4 view = globalubo[nonuniformEXT(push.frameIdx)].view;
	mat4 proj = globalubo[nonuniformEXT(push.frameIdx)].projection;

	mat4 untranslatedView = mat4(mat3(view)); // remove translation
	vec4 clipPos = proj * (untranslatedView * vec4(position, 1.0));

	// giving maximum depth and avoiding z fighting on far plane
	gl_Position = vec4(clipPos.x, clipPos.y, clipPos.w - 0.01, clipPos.w);
}