#include "render_passes.h"

#include "frame_data.h"
#include "vk_swapchain.h"
#include "vk_renderer.h"
#include "engine.h"
#include "ui/editor_ui.h"
#include "debug/profiler.h"

namespace dusk
{
void recordPresentationCmds(const FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    auto& resources = Engine::get().getRenderGraphResources();

    resources.presentPipeline->bind(frameData.commandBuffer);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.presentPipelineLayout->get(),
        0,
        1,
        &frameData.textureDescriptorSet,
        0,
        nullptr);

    PresentationPushConstant push {};
    push.inputTextureIdx = resources.lightingRenderTextureId;

    vkCmdPushConstants(
        frameData.commandBuffer,
        resources.presentPipelineLayout->get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PresentationPushConstant),
        &push);

    vkCmdDraw(frameData.commandBuffer, 3, 1, 0, 0);

    {
        DUSK_PROFILE_SECTION("editor_cmds_recording");

        // record commands for editor ui here
        auto& editorUI = Engine::get().getEditorUI();

        editorUI.beginRendering();
        editorUI.renderCommonWidgets();
        editorUI.renderSceneWidgets(*frameData.scene);
        editorUI.endRendering(frameData.commandBuffer);
    }

    return;
}

} // namespace dusk