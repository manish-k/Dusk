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

    Scene&          scene           = *frameData.scene;
    VkCommandBuffer commandBuffer   = frameData.commandBuffer;

    auto&           resources       = Engine::get().getRenderGraphResources();

    auto            renderablesView = scene.GetGameObjectsWith<MeshComponent>();

    // possiblity of cache unfriendliness here. Only first component is
    // cache friendly. https://gamedev.stackexchange.com/a/212879
    // TODO: Profile below code

    DUSK_PROFILE_SECTION("draw_calls_recording_single_thread");

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

    DynamicArray<GfxIndexedIndirectDrawCommand> indirectDrawCommands;
    DynamicArray<GfxMeshInstanceData>           meshInstanceData;

    indirectDrawCommands.reserve(maxModelCount);
    meshInstanceData.reserve(maxModelCount);

    uint32_t instanceCounter = 0u;
    for (auto& entity : renderablesView)
    {
        auto& meshData = renderablesView.get<MeshComponent>(entity);

        for (uint32_t index = 0u; index < meshData.meshes.size(); ++index)
        {
            uint32_t       materialId = meshData.materials[index];
            const SubMesh& mesh       = scene.getSubMesh(meshData.meshes[index]);
            auto&          transform  = Registry::getRegistry().get<TransformComponent>(entity);

            meshInstanceData.push_back(
                GfxMeshInstanceData {
                    .modelMat   = transform.mat4(),
                    .normalMat  = transform.normalMat4(),
                    .materialId = materialId,
                });

            indirectDrawCommands.push_back(
                GfxIndexedIndirectDrawCommand {
                    .indexCount    = mesh.getIndexCount(),
                    .instanceCount = 1,
                    .firstIndex    = mesh.getIndexBufferIndex(),
                    .vertexOffset  = mesh.getVertexOffset(),
                    .firstInstance = instanceCounter,
                });

            ++instanceCounter;
        }
    }

    auto& currentIndirectBuffer     = resources.frameIndirectDrawCommands[frameData.frameIndex];
    auto& currentMeshInstanceBuffer = resources.meshInstanceDataBuffers[frameData.frameIndex];

    currentMeshInstanceBuffer.writeAndFlushAtIndex(0, meshInstanceData.data(), sizeof(GfxMeshInstanceData) * meshInstanceData.size());

    GfxBuffer indirectStagingBuffer;
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::TransferSource,
        sizeof(GfxIndexedIndirectDrawCommand),
        indirectDrawCommands.size(),
        "staging_index_buffer",
        &indirectStagingBuffer);

    indirectStagingBuffer.writeAndFlushAtIndex(0, indirectDrawCommands.data(), sizeof(GfxIndexedIndirectDrawCommand) * indirectDrawCommands.size());

    currentIndirectBuffer.copyFrom(indirectStagingBuffer, sizeof(GfxIndexedIndirectDrawCommand) * indirectDrawCommands.size());

    indirectStagingBuffer.cleanup();

    {
        DUSK_PROFILE_GPU_ZONE("gbuffer_indirect_draw", commandBuffer);
        // vkCmdDrawIndexed(commandBuffer, mesh.getIndexCount(), 1, 0, 0, 0);

        vkCmdDrawIndexedIndirect(
            commandBuffer,
            currentIndirectBuffer.vkBuffer.buffer,
            0,
            static_cast<uint32_t>(indirectDrawCommands.size()),
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