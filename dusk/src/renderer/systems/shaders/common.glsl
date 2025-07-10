#ifndef COMMON_GLSL
#define COMMON_GLSL


vec3 worldPosFromDepth(vec2 uv, float ndcDepth, mat4 projInverse, mat4 viewInverse)
{
    // remap to [-1.0, 1.0] range.
    vec2 screenPos = uv * 2.0f - 1.0f;

    // NDC position.
    vec4 ndcPos = vec4(screenPos, ndcDepth, 1.0f);

    // world position.
    vec4 viewSpacePos = projInverse * ndcPos;
    viewSpacePos = viewSpacePos / viewSpacePos.w;

	vec4 worldPos = viewInverse * viewSpacePos;

    return worldPos.xyz;
}

#endif