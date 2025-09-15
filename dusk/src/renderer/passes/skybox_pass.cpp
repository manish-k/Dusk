#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"
#include "ui/editor_ui.h"
#include "backend/vulkan/vk_pass.h"
#include "environment.h"

namespace dusk
{
void recordSkyBoxCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx)

{
    if (!EditorUI::state().rendererState.useSkybox) return;

    if (!frameData.scene) return;

    Scene& scene     = *frameData.scene;
    //auto&  resources = Engine::get().getRenderGraphResources();
    auto& environment = Engine::get().getEnvironment();

    environment.getPipeline().bind(ctx.cmdBuffer);

    vkCmdBindDescriptorSets(
        ctx.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        environment.getPipelineLayout().get(),
        0,
        1,
        &frameData.globalDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        ctx.cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        environment.getPipelineLayout().get(),
        1,
        1,
        &frameData.textureDescriptorSet,
        0,
        nullptr);

    SkyBoxPushConstant push {};
    push.frameIdx             = frameData.frameIndex;
    push.skyColorTextureIdx   = environment.getSkyTextureId();

    vkCmdPushConstants(
        ctx.cmdBuffer,
        environment.getPipelineLayout().get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(SkyBoxPushConstant),
        &push);

    VkBuffer     buffers[1] = { environment.getCubeMesh().getVertexBuffer().vkBuffer.buffer };
    VkDeviceSize offsets[1] = { 0 };

    vkCmdBindVertexBuffers(ctx.cmdBuffer, 0, 1, buffers, offsets);

    vkCmdBindIndexBuffer(ctx.cmdBuffer, environment.getCubeMesh().getIndexBuffer().vkBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(ctx.cmdBuffer, environment.getCubeMesh().getIndexCount(), 1, 0, 0, 0);
}

} // namespace dusk