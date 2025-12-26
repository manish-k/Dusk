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
    DynamicArray<GfxMeshInstanceData> meshInstanceData;

    meshInstanceData.reserve(MAX_RENDERABLES_COUNT);

    // possiblity of cache unfriendliness here. Only first component is
    // cache friendly. https://gamedev.stackexchange.com/a/212879
    // TODO: Profile below code
    {
        DUSK_PROFILE_SECTION("gather_mesh_instances");
        auto renderablesView = scene.GetGameObjectsWith<MeshComponent>();
        for (auto& entity : renderablesView)
        {
            auto&     meshData        = renderablesView.get<MeshComponent>(entity);

            auto&     transform       = Registry::getRegistry().get<TransformComponent>(entity);
            glm::mat4 transformMatrix = transform.mat4();
            glm::mat4 normalMatrix    = transform.normalMat4();
            AABB      aabb            = meshData.worldAABB;

            for (uint32_t index = 0u; index < meshData.meshes.size(); ++index)
            {
                meshInstanceData.push_back(
                    GfxMeshInstanceData {
                        .modelMat   = transformMatrix,
                        .normalMat  = normalMatrix,
                        .center     = (aabb.min + aabb.max) * 0.5f,
                        .meshId     = meshData.meshes[index],
                        .extents    = (aabb.max - aabb.min) * 0.5f,
                        .materialId = meshData.materials[index],
                    });
            }
        }

        // sorting by material to improve cache locality in the gpu
        std::sort(
            meshInstanceData.begin(),
            meshInstanceData.end(),
            [](const GfxMeshInstanceData& a, const GfxMeshInstanceData& b)
            {
                return a.materialId < b.materialId;
            });
    }

    {
        DUSK_PROFILE_SECTION("upload_mesh_instance_data");
        auto& currentMeshInstanceBuffer = resources.meshInstanceDataBuffers[frameData.frameIndex];

        currentMeshInstanceBuffer.writeAndFlushAtIndex(0, meshInstanceData.data(), sizeof(GfxMeshInstanceData) * meshInstanceData.size());
    }
    {
        DUSK_PROFILE_SECTION("reset_draw_count_buffer");
        // reset draw count buffer to zero
        auto& currentDrawCountBuffer = resources.frameIndirectDrawCountBuffers[frameData.frameIndex];

        vkCmdFillBuffer(
            commandBuffer,
            currentDrawCountBuffer.vkBuffer.buffer,
            0,
            VK_WHOLE_SIZE,
            0);
    }

    {
        DUSK_PROFILE_SECTION("resource_bindings");

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

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            resources.cullLodPipelineLayout->get(),
            1, // binding location
            1,
            &frameData.meshDataDescriptorSet,
            0,
            nullptr);

        // bind mesh instance data descriptor set
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            resources.cullLodPipelineLayout->get(),
            2, // binding location
            1,
            &resources.meshInstanceDataDescriptorSet[frameData.frameIndex]->set,
            0,
            nullptr);

        // bind indirect draw descriptor set
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            resources.cullLodPipelineLayout->get(),
            3, // binding location
            1,
            &resources.indirectDrawDescriptorSet[frameData.frameIndex]->set,
            0,
            nullptr);
    }

    {
        DUSK_PROFILE_SECTION("dispatch");
        // push constants
        CullLodPushConstant push {};
        push.globalUboIdx = frameData.frameIndex;
        push.objectCount  = meshInstanceData.size();

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
}
} // namespace dusk