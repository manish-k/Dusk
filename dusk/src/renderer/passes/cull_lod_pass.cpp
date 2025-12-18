#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"

#include "scene/scene.h"
#include "scene/components/transform.h"
#include "scene/components/mesh.h"

#include "backend/vulkan/vk_descriptors.h"
#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"

namespace dusk
{
void dispatchIndirectDrawCompute(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    Scene&          scene         = *frameData.scene;
    VkCommandBuffer commandBuffer = frameData.commandBuffer;
    auto&           resources     = Engine::get().getRenderGraphResources();

    // gather all mesh instances in the scene and upload to GPU buffers along with indirect draw commands

    // TODO: mesh buffer updates should go inside scene update()
    DynamicArray<GfxIndexedIndirectDrawCommand> indirectDrawCommands;
    DynamicArray<GfxMeshInstanceData>           meshInstanceData;

    indirectDrawCommands.reserve(maxModelCount);
    meshInstanceData.reserve(maxModelCount);

    // possiblity of cache unfriendliness here. Only first component is
    // cache friendly. https://gamedev.stackexchange.com/a/212879
    // TODO: Profile below code
    uint32_t instanceCounter = 0u;
    auto     renderablesView = scene.GetGameObjectsWith<MeshComponent>();
    for (auto& entity : renderablesView)
    {
        auto& meshData = renderablesView.get<MeshComponent>(entity);

        for (uint32_t index = 0u; index < meshData.meshes.size(); ++index)
        {
            uint32_t       materialId      = meshData.materials[index];
            const SubMesh& mesh            = scene.getSubMesh(meshData.meshes[index]);
            auto&          transform       = Registry::getRegistry().get<TransformComponent>(entity);

            glm::mat4      transformMatrix = transform.mat4();
            AABB           transformedAABB = recomputeAABB(mesh.getAABB(), transformMatrix);

            meshInstanceData.push_back(
                GfxMeshInstanceData {
                    .modelMat   = transformMatrix,
                    .normalMat  = transform.normalMat4(),
                    .aabbMin    = transformedAABB.min,
                    .aabbMax    = transformedAABB.max,
                    .materialId = materialId,
                });

            indirectDrawCommands.push_back(
                GfxIndexedIndirectDrawCommand {
                    .indexCount    = mesh.getIndexCount(),
                    .instanceCount = 0, // compute shader will fill this
                    .firstIndex    = mesh.getIndexBufferIndex(),
                    .vertexOffset  = mesh.getVertexOffset(),
                    .firstInstance = instanceCounter,
                });

            ++instanceCounter;
        }
    }

    auto& currentIndirectBuffer     = resources.frameIndirectDrawCommandsBuffers[frameData.frameIndex];
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

    // reset draw count buffer to zero
    GfxBuffer drawCountStagingBuffer;
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::TransferSource,
        sizeof(GfxIndexedIndirectDrawCount),
        1,
        "staging_draw_count_buffer",
        &drawCountStagingBuffer);

    // TODO: reset value to zero each frame.
    // for testing using instancecounter
    GfxIndexedIndirectDrawCount defaultCount { instanceCounter };
    drawCountStagingBuffer.writeAndFlushAtIndex(0, &defaultCount, sizeof(GfxIndexedIndirectDrawCount));
    auto& currentDrawCountBuffer = resources.frameIndirectDrawCountBuffers[frameData.frameIndex];

    currentDrawCountBuffer.copyFrom(drawCountStagingBuffer, sizeof(GfxIndexedIndirectDrawCount) * 1);

    drawCountStagingBuffer.cleanup();

    // bind cull and lod pipeline
    resources.cullLodPipeline->bind(commandBuffer);

    // bind global descriptor set
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        resources.cullLodPipelineLayout->get(),
        0, // binding location
        1,
        &frameData.globalDescriptorSet,
        0,
        nullptr);

    // bind mesh instance data descriptor set
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        resources.cullLodPipelineLayout->get(),
        1, // binding location
        1,
        &resources.meshInstanceDataDescriptorSet[frameData.frameIndex]->set,
        0,
        nullptr);

    // bind indirect draw descriptor set
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        resources.cullLodPipelineLayout->get(),
        2, // binding location
        1,
        &resources.indirectDrawDescriptorSet[frameData.frameIndex]->set,
        0,
        nullptr);

    // push constants
    CullLodPushConstant push {};
    push.globalUboIdx = frameData.frameIndex;
    push.objectCount  = 100;

    vkCmdPushConstants(
        commandBuffer,
        resources.cullLodPipelineLayout->get(),
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(CullLodPushConstant),
        &push);

    // dispatch compute shader
    uint32_t workgroupCount = (push.objectCount + 31) / 32;
    vkCmdDispatch(
        commandBuffer,
        workgroupCount,
        1,
        1);

    // barrier to ensure compute shader writes are visible to subsequent draw calls
    VkMemoryBarrier memoryBarrier {};
    memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        0,
        1,
        &memoryBarrier,
        0,
        nullptr,
        0,
        nullptr);
}
} // namespace dusk