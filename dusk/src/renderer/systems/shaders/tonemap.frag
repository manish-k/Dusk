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

// Linear sRGB (Rec.709, D65) → ACES AP0
const mat3 ACESInputMat = mat3(
    vec3(0.59719, 0.07600, 0.02840),  // col 0
    vec3(0.35458, 0.90834, 0.13383),  // col 1
    vec3(0.04823, 0.01566, 0.83777)   // col 2
);

// ACES AP1 → linear sRGB
const mat3 ACESOutputMat = mat3(
    vec3( 1.60475, -0.10208, -0.00327),  // col 0
    vec3(-0.53108,  1.10813, -0.07276),  // col 1
    vec3(-0.07367, -0.00605,  1.07602)   // col 2
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

vec3 ReinhardJodie(vec3 color) {
    float luma    = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3  tv      = color / (1.0 + color);           // per-channel
    float lumaOut = luma  / (1.0 + luma);            // luminance
    return mix(color / (1.0 + luma), tv, tv);        // blend
}

void main()
{
    int hdrTextureIdx = nonuniformEXT(push.inputTextureIdx);

    vec3 hdrColor = texture(textures[hdrTextureIdx], fragUV).rgb;

    // exposure
    hdrColor *= push.exposure;

    // tonemap
    // hdrColor = ReinhardJodie(hdrColor);
    hdrColor = ACESFilmic(hdrColor);

    outColor = vec4(hdrColor, 1.0);
}
