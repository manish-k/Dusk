#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform PushConstant 
{
	int inputTextureIdx;
    float exposure;
} push;


// ============================================================
//  ACES Input / Output Transform Matrices
//  Source: Academy S-2014-003 spec
// ============================================================

// sRGB (D65) → ACES AP0 (D60)
const mat3 ACESInputMat = mat3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

// ACES AP1 → sRGB (D65)
const mat3 ACESOutputMat = mat3(
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

// ============================================================
//  RRT + ODT (Reference Rendering Transform + Output Device)
//  Fitted approximation by Stephen Hill (@self_shadow)
// ============================================================
vec3 RRTAndODTFit(vec3 v) {
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACESFilmic(vec3 color) {
    color = ACESInputMat * color;
    color = RRTAndODTFit(color);
    color = ACESOutputMat * color;
    return clamp(color, 0.0, 1.0);
}

void main()
{
    int hdrTextureIdx = nonuniformEXT(push.inputTextureIdx);

    vec3 hdrColor = texture(textures[hdrTextureIdx], fragUV).rgb;

    // exposure
    hdrColor *= push.exposure;

    // tonemap
    hdrColor = ACESFilmic(hdrColor);

    outColor = vec4(hdrColor, 1.0);
}
