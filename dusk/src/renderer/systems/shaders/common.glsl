#ifndef COMMON_GLSL
#define COMMON_GLSL


vec3 worldPosFromDepth(vec2 uv, float ndcDepth, mat4 viewProjInverse)
{
    // remap to [-1.0, 1.0] range.
    vec2 screenPos = uv * 2.0f - 1.0f;

    // NDC position.
    vec4 ndcPos = vec4(screenPos, ndcDepth, 1.0f);

    // world position.
    vec4 worldPos = viewProjInverse * ndcPos;
    worldPos = worldPos / worldPos.w;

    return worldPos.xyz;
}

#endif