#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"

#include "scene/scene.h"
#include "scene/components/transform.h"
#include "scene/components/mesh.h"
#include "scene/components/lights.h"

// TODO: https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-17-efficient-soft-edged-shadows-using for softer shadows

namespace dusk
{
void recordShadow2DMapsCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    Scene&          scene         = *frameData.scene;
    VkCommandBuffer commandBuffer = ctx.cmdBuffer;

    const auto&     resources     = Engine::get().getRenderGraphResources();
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
        DUSK_PROFILE_SECTION("shadow_bind_vertex", commandBuffer);

        // TODO:: getter for index and vertex buffer is ugly
        VkBuffer     buffers[] = { Engine::get().getVertexBuffer().vkBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, Engine::get().getIndexBuffer().vkBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    {
        DUSK_PROFILE_SECTION("shadow_map_draw", commandBuffer);

        auto& submeshes      = scene.getSubMeshes();
        auto  renderables    = frameData.renderables;
        auto  totalInstnaces = static_cast<uint32_t>(renderables->meshIds.size());

        for (uint32_t instanceIdx = 0u; instanceIdx < totalInstnaces; ++instanceIdx)
        {
            uint32_t       meshId = renderables->meshIds[instanceIdx];
            const SubMesh& mesh   = submeshes[meshId];

            vkCmdDrawIndexed(
                commandBuffer,
                mesh.getIndexCount(),
                1,                          // instance count
                mesh.getIndexBufferIndex(), // firstIndex
                mesh.getVertexOffset(),     // vertexOffset
                instanceIdx);               // firstInstance
        }
    }
}

} // namespace dusk