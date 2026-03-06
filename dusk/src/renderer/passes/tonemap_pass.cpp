#include "render_passes.h"

#include "frame_data.h"
#include "vk_swapchain.h"
#include "vk_renderer.h"
#include "engine.h"
#include "ui/editor_ui.h"
#include "debug/profiler.h"

namespace dusk
{
void recordTonemapCmds(const FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    const auto& resources = Engine::get().getRenderGraphResources();

    resources.toneMapPipeline->bind(frameData.commandBuffer);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.toneMapPipelineLayout->get(),
        0,
        1,
        &frameData.textureDescriptorSet,
        0,
        nullptr);

    ToneMapPushConstant push {};
    push.inputTextureIdx = resources.lightingRenderTextureId;

    vkCmdPushConstants(
        frameData.commandBuffer,
        resources.toneMapPipelineLayout->get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(ToneMapPushConstant),
        &push);

    vkCmdDraw(frameData.commandBuffer, 3, 1, 0, 0);

    return;
}

} // namespace dusk