#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"
#include "ui/editor_ui.h"
#include "backend/vulkan/vk_pass.h"

namespace dusk
{
void recordSkyBoxCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx)

{
    if (!EditorUI::state().rendererState.useSkybox) return;

    if (!frameData.scene) return;

    Scene& scene     = *frameData.scene;
    auto&  resources = Engine::get().getRenderGraphResources();

    resources.skyBoxPipeline->bind(ctx.cmdBuffer);

    vkCmdBindDescriptorSets(
        ctx.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.skyBoxPipelineLayout->get(),
        0,
        1,
        &frameData.globalDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        ctx.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.skyBoxPipelineLayout->get(),
        1,
        1,
        &frameData.textureDescriptorSet,
        0,
        nullptr);

    SkyBoxPushConstant push {};
    push.frameIdx             = frameData.frameIndex;
    push.skyColorTextureIdx   = resources.skyTextureId;

    vkCmdPushConstants(
        ctx.cmdBuffer,
        resources.skyBoxPipelineLayout->get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(SkyBoxPushConstant),
        &push);

    VkBuffer     buffers[1] = { resources.cubeMesh->getVertexBuffer().vkBuffer.buffer };
    VkDeviceSize offsets[1] = { 0 };

    vkCmdBindVertexBuffers(ctx.cmdBuffer, 0, 1, buffers, offsets);

    vkCmdBindIndexBuffer(ctx.cmdBuffer, resources.cubeMesh->getIndexBuffer().vkBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(ctx.cmdBuffer, resources.cubeMesh->getIndexCount(), 1, 0, 0, 0);
}

} // namespace dusk