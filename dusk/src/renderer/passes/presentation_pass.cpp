#include "render_passes.h"

#include "frame_data.h"
#include "vk_pass.h"
#include "vk_swapchain.h"
#include "vk_renderer.h"
#include "engine.h"

namespace dusk
{
void recordPresentationCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx)
{
    if (!frameData.scene) return;

    auto& resources = Engine::get().getRenderGraphResources();

    resources.presentPipeline->bind(frameData.commandBuffer);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.presentPipelineLayout->get(),
        0,
        1,
        &resources.presentTexDescriptorSet->set,
        0,
        nullptr);

    vkCmdDraw(frameData.commandBuffer, 3, 1, 0, 0);

    return;
}

} // namespace dusk