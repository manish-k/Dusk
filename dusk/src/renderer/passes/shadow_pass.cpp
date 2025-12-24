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

    Scene&          scene           = *frameData.scene;
    VkCommandBuffer commandBuffer   = frameData.commandBuffer;

    auto&           resources       = Engine::get().getRenderGraphResources();

    auto            renderablesView = scene.GetGameObjectsWith<MeshComponent>();

    resources.shadow2DMapPipeline->bind(commandBuffer);

    // global descriptor set
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.shadow2DMapPipelineLayout->get(),
        0, // global desc set binding location
        1,
        &frameData.globalDescriptorSet,
        0,
        nullptr);

    // model descriptor set
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.shadow2DMapPipelineLayout->get(),
        1, // model desc set binding location
        1,
        &resources.gbuffModelDescriptorSet[frameData.frameIndex]->set,
        0,
        nullptr);

    // lights descriptor set
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        resources.shadow2DMapPipelineLayout->get(),
        2,
        1,
        &frameData.lightsDescriptorSet,
        0,
        nullptr);

    {
        DUSK_PROFILE_GPU_ZONE("shadow_bind_vertex", commandBuffer);

        // TODO:: getter for index and vertex buffer is ugly
        VkBuffer     buffers[] = { Engine::get().getVertexBuffer().vkBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, Engine::get().getIndexBuffer().vkBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    {
        DUSK_PROFILE_GPU_ZONE("shadow_map_draw", commandBuffer);

        for (auto& entity : renderablesView)
        {
            entt::id_type objectId = static_cast<entt::id_type>(entity);
            auto&         meshData = renderablesView.get<MeshComponent>(entity);

            for (uint32_t index = 0u; index < meshData.meshes.size(); ++index)
            {
                const SubMesh&        mesh = scene.getSubMesh(meshData.meshes[index]);

                ShadowMapPushConstant push {};
                push.frameIdx = frameData.frameIndex;
                push.modelIdx = objectId;

                vkCmdPushConstants(
                    commandBuffer,
                    resources.gbuffPipelineLayout->get(),
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(ShadowMapPushConstant),
                    &push);

                {
                    vkCmdDrawIndexed(
                        commandBuffer,
                        mesh.getIndexCount(),
                        1,                          // instance count
                        mesh.getIndexBufferIndex(), // firstIndex
                        mesh.getVertexOffset(),     // vertexOffset
                        0);                         // firstInstance
                }
            }
        }
    }
}

} // namespace dusk