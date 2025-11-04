#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragUVW;

layout(location = 0) out vec4 outColor;

layout (set = 1, binding = 0) uniform sampler2D textures[];

const float PI = 3.14159265359;

layout(push_constant) uniform SkyBoxPushConstant 
{
	uint frameIdx;
    int skyColorTextureIdx;
} push;

// Convert 3D direction to equirectangular UV coordinates
vec2 directionToEquirectangular(vec3 dir) {
    vec2 uv;
    uv.x = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    uv.y = asin(dir.y) / PI + 0.5;
    return uv;
}

void main()
{
    int skyTexIdx = nonuniformEXT(push.skyColorTextureIdx);
    
    vec3 direction = normalize(fragUVW);
    vec2 uv = directionToEquirectangular(direction);

    vec3 skyColor    = textureLod(textures[skyTexIdx], uv, 0).rgb;

    // tone mapping
	skyColor = skyColor / (skyColor + vec3(1.0));

    outColor = vec4(skyColor, 1.0);
}
