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
    int32_t albedoTextureIdx;
    int32_t normalTextureIdx;
    int32_t aoRoughMetalTextureIdx;
    int32_t depthTextureIdx;
};

void recordLightingCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);

//////////////////////////////////////////////////////
// Skybox Pass
struct SkyBoxPushConstant
{
    uint32_t frameIdx;
    int32_t skyColorTextureIdx;
};

void recordSkyBoxCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);

//////////////////////////////////////////////////////
// Presentation Pass

struct PresentationPushConstant
{
    int32_t inputTextureIdx;
};

void recordPresentationCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);
} // namespace dusk
