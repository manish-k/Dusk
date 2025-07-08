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
    uint32_t albedoTextureIdx;
    uint32_t normalTextureIdx;
    uint32_t depthTextureIdx;
};

void recordLightingCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);

//////////////////////////////////////////////////////
// Presentation Pass

struct PresentationPushConstant
{
    uint32_t inputTextureIdx;
};

void recordPresentationCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);
} // namespace dusk
