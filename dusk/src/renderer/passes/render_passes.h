#pragma once

#include "dusk.h"

namespace dusk
{
struct FrameData;
struct VkGfxRenderPassContext;

struct PresentationPushConstant
{
    uint32_t inputTextureIdx;
};

void recordGBufferCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);

void recordPresentationCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);
} // namespace dusk
