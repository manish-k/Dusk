#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"

#include "scene/scene.h"
#include "scene/components/transform.h"
#include "scene/components/mesh.h"

#include "renderer/sub_mesh.h"
#include "renderer/systems/basic_render_system.h"

#include "backend/vulkan/vk_descriptors.h"
#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"

#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>

namespace dusk
{
void recordGBufferCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    Scene&          scene         = *frameData.scene;
    VkCommandBuffer commandBuffer = frameData.commandBuffer;
    auto&           resources     = Engine::get().getRenderGraphResources();

    resources.gbuffPipeline->bind(commandBuffer);

    // TODO:: this might be required only once, check case
    // where new textures are added on the fly (streaming textures)
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
            &resources.meshInstanceDataDescriptorSet[frameData.frameIndex]->set,
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

    DrawData push {};
    push.cameraBufferIdx = frameData.frameIndex;

    vkCmdPushConstants(
        commandBuffer,
        resources.gbuffPipelineLayout->get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(DrawData),
        &push);

    auto&    currentIndirectBuffer          = resources.frameIndirectDrawCommandsBuffers[frameData.frameIndex];
    auto&    currentIndirectDrawCountBuffer = resources.frameIndirectDrawCountBuffers[frameData.frameIndex];

    {
        DUSK_PROFILE_GPU_ZONE("gbuffer_indirect_draw", commandBuffer);

        vkCmdDrawIndexedIndirectCount(
            commandBuffer,
            currentIndirectBuffer.vkBuffer.buffer,
            0,
            currentIndirectDrawCountBuffer.vkBuffer.buffer,
            0,
            10000,
            sizeof(GfxIndexedIndirectDrawCommand));
    }
}

void recordGBufferCmdsMutliThreaded(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    Scene&                     scene           = *frameData.scene;
    VkCommandBuffer            commandBuffer   = frameData.commandBuffer;
    auto&                      resources       = Engine::get().getRenderGraphResources();
    auto                       renderablesView = scene.GetGameObjectsWith<MeshComponent>();

    tf::Executor               executor(ctx.maxParallelism);
    tf::Taskflow               taskflow;

    DynamicArray<entt::entity> renderables(renderablesView.begin(), renderablesView.end());
    uint32_t                   stepSize = renderables.size() / ctx.maxParallelism;
    tf::IndexRange<int>        range(0, renderables.size(), std::max(stepSize, 1u));

    taskflow.for_each_by_index(
        range,
        [&](tf::IndexRange<int> subrange)
        {
            int             workerId       = executor.this_worker_id();
            VkCommandBuffer secondayBuffer = ctx.secondaryCmdBuffers[workerId];

            resources.gbuffPipeline->bind(secondayBuffer);

            // TODO:: this might be required only once, check case
            // where new textures are added on the fly (streaming textures)
            {
                DUSK_PROFILE_GPU_ZONE("gbuffer_bind_desc_set", secondayBuffer);
                vkCmdBindDescriptorSets(
                    secondayBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    resources.gbuffPipelineLayout->get(),
                    0, // global desc set binding location
                    1,
                    &frameData.globalDescriptorSet,
                    0,
                    nullptr);

                vkCmdBindDescriptorSets(
                    secondayBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    resources.gbuffPipelineLayout->get(),
                    1, // material desc set binding location
                    1,
                    &frameData.materialDescriptorSet,
                    0,
                    nullptr);

                vkCmdBindDescriptorSets(
                    secondayBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    resources.gbuffPipelineLayout->get(),
                    2, // model desc set binding location
                    1,
                    &resources.gbuffModelDescriptorSet[frameData.frameIndex]->set,
                    0,
                    nullptr);

                vkCmdBindDescriptorSets(
                    secondayBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    resources.gbuffPipelineLayout->get(),
                    3, // texture desc set binding location
                    1,
                    &frameData.textureDescriptorSet,
                    0,
                    nullptr);
            }

            for (int index = subrange.begin(); index < subrange.end() && index < renderablesView.size(); index += 1)
            {
                entt::entity  entity   = renderables[index];

                entt::id_type objectId = static_cast<entt::id_type>(entity);
                auto&         meshData = renderablesView.get<MeshComponent>(entity);

                for (uint32_t index = 0u; index < meshData.meshes.size(); ++index)
                {
                    int32_t      meshId     = meshData.meshes[index];
                    int32_t      materialId = meshData.materials[index];

                    SubMesh&     mesh       = scene.getSubMesh(meshId);
                    VkBuffer     buffers[]  = { mesh.getVertexBuffer().vkBuffer.buffer };
                    VkDeviceSize offsets[]  = { 0 };

                    DrawData     push {};
                    push.cameraBufferIdx = frameData.frameIndex;
                    push.materialIdx     = materialId;
                    push.modelIdx        = objectId;

                    vkCmdPushConstants(
                        secondayBuffer,
                        resources.gbuffPipelineLayout->get(),
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0,
                        sizeof(DrawData),
                        &push);

                    {
                        DUSK_PROFILE_GPU_ZONE("gbuffer_bind_vertex", secondayBuffer);
                        vkCmdBindVertexBuffers(secondayBuffer, 0, 1, buffers, offsets);

                        vkCmdBindIndexBuffer(secondayBuffer, mesh.getIndexBuffer().vkBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    }

                    {
                        DUSK_PROFILE_GPU_ZONE("gbuffer_draw", secondayBuffer);
                        vkCmdDrawIndexed(secondayBuffer, mesh.getIndexCount(), 1, 0, 0, 0);
                    }
                }
            }
        });

    executor.run(taskflow).wait();
}
} // namespace dusk