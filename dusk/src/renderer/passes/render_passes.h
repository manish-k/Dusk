#pragma once

#include "dusk.h"

#include "backend/vulkan/vk_pass.h"

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
    int32_t  prefilteredTextureIdx  = -1;
    int32_t  maxPrefilteredLODs     = -1;
    int32_t  brdfLUTIdx             = -1;
    int32_t  dirShadowMapTextureIdx = -1;
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

//////////////////////////////////////////////////////
// Shadow Pass

struct ShadowMapPushConstant
{
    uint32_t frameIdx = 0u;
    uint32_t modelIdx = 0u;
};

void recordShadow2DMapsCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);

//////////////////////////////////////////////////////
// Cull & LOD Pass

struct CullLodPushConstant
{
    uint32_t globalUboIdx;
    uint32_t objectCount;
};

void dispatchIndirectDrawCompute(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx);

} // namespace dusk
