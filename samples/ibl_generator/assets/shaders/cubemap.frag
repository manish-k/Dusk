#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragUVW;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D textures[];

const float PI = 3.14159265359;

// Convert 3D direction to equirectangular UV coordinates
vec2 directionToEquirectangular(vec3 dir) {
    vec2 uv;
    uv.x = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    uv.y = asin(dir.y) / PI + 0.5;
    return uv;
}

layout(push_constant) uniform CubeMapPushConstant {
    uint equiRectTexIdx;
} push;

void main()
{
    uint equirectTexIdx = nonuniformEXT(push.equiRectTexIdx);

    vec3 direction = normalize(fragUVW);
    vec2 uv = directionToEquirectangular(direction);

    vec3 color    = texture(textures[equirectTexIdx], uv).rgb;

    outColor = vec4(color, 1.0);
}