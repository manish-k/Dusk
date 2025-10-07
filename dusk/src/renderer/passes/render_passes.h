#pragma once

#include "dusk.h"

namespace dusk
{
struct FrameData;
struct VkGfxRenderPassContext;

//////////////////////////////////////////////////////
// G-Buffer Pass

void recordGBufferCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);

//////////////////////////////////////////////////////
// Lighting Pass

struct LightingPushConstant
{
    uint32_t frameIdx;
    int32_t  albedoTextureIdx       = -1;
    int32_t  normalTextureIdx       = -1;
    int32_t  aoRoughMetalTextureIdx = -1;
    int32_t  emissiveTextureIdx     = -1;
    int32_t  depthTextureIdx        = -1;
    int32_t  irradianceTextureIdx   = -1;
    int32_t  radianceTextureIdx     = -1;
    int32_t  maxRadianceLODs        = -1;
    int32_t  brdfLUTIdx             = -1;
};

void recordLightingCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);

//////////////////////////////////////////////////////
// Skybox Pass
struct SkyBoxPushConstant
{
    uint32_t frameIdx           = 0u;
    int32_t  skyColorTextureIdx = -1;
};

void recordSkyBoxCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);

//////////////////////////////////////////////////////
// Presentation Pass

struct PresentationPushConstant
{
    int32_t inputTextureIdx = -1;
};

void recordPresentationCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);
} // namespace dusk
