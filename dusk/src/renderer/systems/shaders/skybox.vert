#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec3 fragUVW;

// Cube vertices
const vec3 positions[8] = vec3[8](
    vec3(-1.0, -1.0, -1.0),
    vec3( 1.0, -1.0, -1.0),
    vec3( 1.0,  1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3(-1.0, -1.0,  1.0),
    vec3( 1.0, -1.0,  1.0),
    vec3( 1.0,  1.0,  1.0),
    vec3(-1.0,  1.0,  1.0)
);

// Cube indices
const uint indices[36] = uint[36](
    0, 1, 2, 2, 3, 0, // front
    1, 5, 6, 6, 2, 1, // right
    5, 4, 7, 7, 6, 5, // back
    4, 0, 3, 3, 7, 4, // left
    3, 2, 6, 6, 7, 3, // top
    4, 5, 1, 1, 0, 4  // bottom
);

layout (set = 0, binding = 0) uniform GlobalUBO 
{
	mat4 projection;
	mat4 view;
	mat4 inverseView;
	
	vec4 frustumPlanes[6];
	
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
	uint idx = indices[gl_VertexIndex];

	mat4 view = globalubo[nonuniformEXT(push.frameIdx)].view;
	mat4 proj = globalubo[nonuniformEXT(push.frameIdx)].projection;

	mat4 untranslatedView = mat4(mat3(view)); // remove translation
	vec4 clipPos = proj * (untranslatedView * vec4(positions[idx], 1.0));

	fragUVW = positions[idx];

	// giving maximum depth
	gl_Position = clipPos.xyww;
}