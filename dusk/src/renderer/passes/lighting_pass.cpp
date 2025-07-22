#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"

#include "backend/vulkan/vk_pass.h"

namespace dusk
{
void recordLightingCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    auto& resources = Engine::get().getRenderGraphResources();

    resources.lightingPipeline->bind(ctx.cmdBuffer);

    vkCmdBindDescriptorSets(
        ctx.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.lightingPipelineLayout->get(),
        0,
        1,
        &frameData.globalDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        ctx.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.lightingPipelineLayout->get(),
        1,
        1,
        &frameData.textureDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        ctx.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.lightingPipelineLayout->get(),
        2,
        1,
        &frameData.lightsDescriptorSet,
        0,
        nullptr);

    LightingPushConstant push {};
    push.frameIdx               = frameData.frameIndex;
    push.albedoTextureIdx       = ctx.readAttachments[0].texture->id;
    push.normalTextureIdx       = ctx.readAttachments[1].texture->id;
    push.aoRoughMetalTextureIdx = ctx.readAttachments[2].texture->id;
    push.depthTextureIdx        = ctx.readAttachments[2].texture->id;

    vkCmdPushConstants(
        ctx.cmdBuffer,
        resources.lightingPipelineLayout->get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(LightingPushConstant),
        &push);

    vkCmdDraw(ctx.cmdBuffer, 3, 1, 0, 0);
}
} // namespace dusk