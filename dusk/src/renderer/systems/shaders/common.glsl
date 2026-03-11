#ifndef COMMON_GLSL
#define COMMON_GLSL

#ifndef PI
#define PI 3.14159265359
#endif

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


vec2 directionToEquirectangular(vec3 dir) 
{
    vec2 uv;
    uv.x = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    uv.y = asin(dir.y) / PI + 0.5;
    return uv;
}

vec3 getCubeDirection(vec2 uv, int face)
{
    uv = uv * 2.0 - 1.0;

    if(face == 0) return normalize(vec3( 1.0, -uv.y, -uv.x));
    if(face == 1) return normalize(vec3(-1.0, -uv.y,  uv.x));
    if(face == 2) return normalize(vec3( uv.x,  1.0,  uv.y));
    if(face == 3) return normalize(vec3( uv.x, -1.0, -uv.y));
    if(face == 4) return normalize(vec3( uv.x, -uv.y,  1.0));
    return normalize(vec3(-uv.x, -uv.y, -1.0));
}


#endif