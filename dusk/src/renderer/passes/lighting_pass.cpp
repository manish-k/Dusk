#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"
#include "renderer/environment.h"

namespace dusk
{
void recordLightingCmds(const FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    auto& resources = Engine::get().getRenderGraphResources();
    auto& env       = Engine::get().getEnvironment();

    auto  cmdBuffer = frameData.commandBuffer;

    resources.lightingPipeline->bind(cmdBuffer);

    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.lightingPipelineLayout->get(),
        0,
        1,
        &frameData.globalDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.lightingPipelineLayout->get(),
        1,
        1,
        &frameData.textureDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.lightingPipelineLayout->get(),
        2,
        1,
        &frameData.lightsDescriptorSet,
        0,
        nullptr);

    LightingPushConstant push {};
    push.globalUboIdx = frameData.frameIndex;

    // TODO: Need a better way to handle attachment indices, too much indirection
    push.albedoTextureIdx       = resources.gbuffRenderTextureIds[0];
    push.normalTextureIdx       = resources.gbuffRenderTextureIds[1];
    push.aoRoughMetalTextureIdx = resources.gbuffRenderTextureIds[2];
    push.emissiveTextureIdx     = resources.gbuffRenderTextureIds[3];
    push.depthTextureIdx        = resources.gbuffDepthTextureId;
    push.irradianceTextureIdx   = env.getSkyIrradianceTextureId();
    push.prefilteredTextureIdx  = env.getSkyPrefilteredTextureId();
    push.maxPrefilteredLODs     = env.getSkyPrefilteredMaxLods();
    push.brdfLUTIdx             = resources.brdfLUTextureId;
    push.dirShadowMapTextureIdx = resources.dirShadowMapsTextureId;

    vkCmdPushConstants(
        cmdBuffer,
        resources.lightingPipelineLayout->get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(LightingPushConstant),
        &push);

    {
        DUSK_PROFILE_GPU_ZONE("lighting_draw", cmdBuffer);
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    }
}
} // namespace dusk