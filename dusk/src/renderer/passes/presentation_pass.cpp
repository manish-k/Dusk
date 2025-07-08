#include "render_passes.h"

#include "frame_data.h"
#include "vk_pass.h"
#include "vk_swapchain.h"
#include "vk_renderer.h"
#include "engine.h"
#include "ui/editor_ui.h"

namespace dusk
{
void recordPresentationCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx)
{
    if (!frameData.scene) return;

    auto& resources = Engine::get().getRenderGraphResources();

    resources.presentPipeline->bind(ctx.cmdBuffer);

    vkCmdBindDescriptorSets(
        ctx.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.presentPipelineLayout->get(),
        0,
        1,
        &frameData.textureDescriptorSet,
        0,
        nullptr);

    PresentationPushConstant push {};
    push.inputTextureIdx = ctx.inAttachments[0].texture.id;

    vkCmdPushConstants(
        ctx.cmdBuffer,
        resources.presentPipelineLayout->get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PresentationPushConstant),
        &push);

    vkCmdDraw(ctx.cmdBuffer, 3, 1, 0, 0);

    // record commands for editor ui here
    auto& editorUI = Engine::get().getEditorUI();

    editorUI.beginRendering();
    editorUI.renderCommonWidgets();
    editorUI.renderSceneWidgets(*frameData.scene);
    editorUI.endRendering(ctx.cmdBuffer);

    return;
}

} // namespace dusk