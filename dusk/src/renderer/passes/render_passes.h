#pragma once

namespace dusk
{
struct FrameData;
struct VkGfxRenderPassContext;

void recordGBufferCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);

void recordPresentationCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);
} // namespace dusk
