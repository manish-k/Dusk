#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"

#include "scene/scene.h"

// TODO: https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-17-efficient-soft-edged-shadows-using for softer shadows

namespace dusk
{
void recordShadow2DMapsCmds(const FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    Scene&          scene         = *frameData.scene;
    VkCommandBuffer commandBuffer = frameData.commandBuffer;

    auto&           resources     = Engine::get().getRenderGraphResources();
    resources.shadow2DMapPipeline->bind(commandBuffer);

    // renderable list descriptor set
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.shadow2DMapPipelineLayout->get(),
        0, // renderable desc set binding location
        1,
        &frameData.renderablesDescriptorSet,
        0,
        nullptr);

    // lights descriptor set
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.shadow2DMapPipelineLayout->get(),
        1, // binding location
        1,
        &frameData.lightsDescriptorSet,
        0,
        nullptr);

    {
        DUSK_PROFILE_SECTION("shadow_bind_vertex");

        // TODO:: getter for index and vertex buffer is ugly
        VkBuffer     buffers[] = { Engine::get().getVertexBuffer().vkBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, Engine::get().getIndexBuffer().vkBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    GfxBuffer& frameIndirectBuffer = resources.frameIndirectDrawCommandsBuffers[frameData.frameIndex];
    auto&      meshesData          = scene.m_sceneMeshes;
    auto       renderables         = frameData.renderables;
    auto       totalInstnaces      = static_cast<uint32_t>(renderables->meshIds.size());

    {
        DUSK_PROFILE_SECTION("shadow_indirect_buffer_prep");
        DynamicArray<GfxIndexedIndirectDrawCommand> indirectCmdsBuffer = {};

        // record cmds in buffer
        indirectCmdsBuffer.reserve(totalInstnaces);

        for (uint32_t instanceIdx = 0u; instanceIdx < totalInstnaces; ++instanceIdx)
        {
            uint32_t    meshId   = renderables->meshIds[instanceIdx];
            const auto& meshData = meshesData[meshId];

            indirectCmdsBuffer.push_back(
                { .indexCount    = meshData.indexCount,
                  .instanceCount = 1u,
                  .firstIndex    = meshData.firstIndex,
                  .vertexOffset  = meshData.vertexOffset,
                  .firstInstance = instanceIdx });
        }

        GfxBuffer stagingBuffer;
        size_t    stagingBufferSize = totalInstnaces * sizeof(GfxIndexedIndirectDrawCommand);
        GfxBuffer::createHostWriteBuffer(
            GfxBufferUsageFlags::TransferSource,
            stagingBufferSize,
            1,
            "staging_indirect_index_buffer",
            &stagingBuffer);

        // upload indirect cmds buffer
        stagingBuffer.writeAndFlush(0, (void*)indirectCmdsBuffer.data(), stagingBufferSize);

        frameIndirectBuffer.copyFrom(
            stagingBuffer,
            0,
            MAX_RENDERABLES_COUNT * sizeof(GfxIndexedIndirectDrawCommand),
            stagingBufferSize);

        stagingBuffer.cleanup();
    }

    {
        DUSK_PROFILE_SECTION("shadow_map_draw");

        vkCmdDrawIndexedIndirect(
            commandBuffer,
            frameIndirectBuffer.vkBuffer.buffer,
            MAX_RENDERABLES_COUNT * sizeof(GfxIndexedIndirectDrawCommand),
            totalInstnaces,
            sizeof(GfxIndexedIndirectDrawCommand));
    }
}

} // namespace dusk