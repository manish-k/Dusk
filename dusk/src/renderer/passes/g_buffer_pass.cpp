#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"

#include "scene/scene.h"

#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"

namespace dusk
{
void recordGBufferCmds(const FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    const Scene&    scene         = *frameData.scene;
    VkCommandBuffer commandBuffer = frameData.commandBuffer;
    auto&           resources     = Engine::get().getRenderGraphResources();

    resources.gbuffPipeline->bind(commandBuffer);

    {
        DUSK_PROFILE_GPU_ZONE("gbuffer_bind_desc_set", commandBuffer);
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            resources.gbuffPipelineLayout->get(),
            0, // global desc set binding location
            1,
            &frameData.globalDescriptorSet,
            0,
            nullptr);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            resources.gbuffPipelineLayout->get(),
            1, // material desc set binding location
            1,
            &frameData.materialDescriptorSet,
            0,
            nullptr);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            resources.gbuffPipelineLayout->get(),
            2, // mesh instance data desc set binding location
            1,
            &frameData.renderablesDescriptorSet,
            0,
            nullptr);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            resources.gbuffPipelineLayout->get(),
            3, // texture desc set binding location
            1,
            &frameData.textureDescriptorSet,
            0,
            nullptr);
    }

    {
        // TODO:: getter for index and vertex buffer is ugly
        VkBuffer     buffers[] = { Engine::get().getVertexBuffer().vkBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };

        DUSK_PROFILE_GPU_ZONE("gbuffer_bind_vertex", commandBuffer);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, Engine::get().getIndexBuffer().vkBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    GbufferPushConstant push {};
    push.globalUboIdx = frameData.frameIndex;

    vkCmdPushConstants(
        commandBuffer,
        resources.gbuffPipelineLayout->get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(GbufferPushConstant),
        &push);

    auto& currentIndirectBuffer          = resources.frameIndirectDrawCommandsBuffers[frameData.frameIndex];
    auto& currentIndirectDrawCountBuffer = resources.frameIndirectDrawCountBuffers[frameData.frameIndex];

    {
        DUSK_PROFILE_GPU_ZONE("gbuffer_indirect_draw", commandBuffer);

        vkCmdDrawIndexedIndirectCount(
            commandBuffer,
            currentIndirectBuffer.vkBuffer.buffer,
            0,
            currentIndirectDrawCountBuffer.vkBuffer.buffer,
            0,
            MAX_RENDERABLES_COUNT,
            sizeof(GfxIndexedIndirectDrawCommand));
    }
}
} // namespace dusk