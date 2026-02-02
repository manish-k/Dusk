#pragma once

#include "dusk.h"

namespace dusk
{
struct FrameData;
struct VkGfxRenderPassContext;

//////////////////////////////////////////////////////
// G-Buffer Pass
struct GbufferPushConstant
{
    uint32_t globalUboIdx;
};

void recordGBufferCmds(const FrameData& frameData);

//////////////////////////////////////////////////////
// Lighting Pass

struct LightingPushConstant
{
    uint32_t globalUboIdx;
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

void recordLightingCmds(const FrameData& frameData);

//////////////////////////////////////////////////////
// Skybox Pass
struct SkyBoxPushConstant
{
    uint32_t globalUboIdx       = 0u;
    int32_t  skyColorTextureIdx = -1;
};

void recordSkyBoxCmds(const FrameData& frameData);

//////////////////////////////////////////////////////
// Presentation Pass

struct PresentationPushConstant
{
    int32_t inputTextureIdx = -1;
};

void recordPresentationCmds(const FrameData& frameData);

//////////////////////////////////////////////////////
// Shadow Pass

void recordShadow2DMapsCmds(const FrameData& frameData);

//////////////////////////////////////////////////////
// Cull & LOD Pass

struct CullLodPushConstant
{
    uint32_t globalUboIdx;
    uint32_t objectCount;
};

void dispatchIndirectDrawCompute(const FrameData& frameData);

} // namespace dusk
