#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"

#include "scene/scene.h"
#include "scene/components/transform.h"
#include "scene/components/mesh.h"
#include "scene/components/camera.h"

#include "renderer/sub_mesh.h"
#include "renderer/systems/basic_render_system.h"

#include "backend/vulkan/vk_allocator.h"
#include "backend/vulkan/vk_renderer.h"
#include "backend/vulkan/vk_descriptors.h"
#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"
#include "backend/vulkan/vk_pass.h"

#include <taskflow/taskflow.hpp>

namespace dusk
{
void recordGBufferCmds(
    FrameData&              frameData,
    VkGfxRenderPassContext& ctx)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    Scene&           scene         = *frameData.scene;
    VkCommandBuffer  commandBuffer = frameData.commandBuffer;
    CameraComponent& camera        = scene.getMainCamera();

    auto&            resources     = Engine::get().getRenderGraphResources();

    resources.gbuffPipeline->bind(commandBuffer);

    // TODO:: this might be required only once, check case
    // where new textures are added on the fly (streaming textures)
    {
        DUSK_PROFILE_GPU_ZONE("gbuffer_bind_descset", commandBuffer);
        vkCmdBindDescriptorSets(
            frameData.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            resources.gbuffPipelineLayout->get(),
            GLOBAL_SET_INDEX,
            1,
            &frameData.globalDescriptorSet,
            0,
            nullptr);

        vkCmdBindDescriptorSets(
            frameData.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            resources.gbuffPipelineLayout->get(),
            MATERIAL_SET_INDEX,
            1,
            &frameData.materialDescriptorSet,
            0,
            nullptr);

        vkCmdBindDescriptorSets(
            frameData.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            resources.gbuffPipelineLayout->get(),
            LIGHTS_SET_INDEX,
            1,
            &frameData.lightsDescriptorSet,
            0,
            nullptr);
    }

    auto renderablesView = scene.GetGameObjectsWith<TransformComponent, MeshComponent>();
    renderablesView.size_hint();

    // possiblity of cache unfriendliness here. Only first component is
    // cache friendly. https://gamedev.stackexchange.com/a/212879
    // TODO: Profile below code
    for (auto [entity, transform, meshData] : renderablesView.each())
    {
        for (uint32_t index = 0u; index < meshData.meshes.size(); ++index)
        {
            entt::id_type objectId   = static_cast<entt::id_type>(entity);
            int32_t       meshId     = meshData.meshes[index];
            int32_t       materialId = meshData.materials[index];

            SubMesh&      mesh       = scene.getSubMesh(meshId);
            VkBuffer      buffers[]  = { mesh.getVertexBuffer().vkBuffer.buffer };
            VkDeviceSize  offsets[]  = { 0 };

            DrawData      push {};
            push.cameraBufferIdx = frameData.frameIndex;
            push.materialIdx     = materialId;

            // update mesh transform data
            ModelData md { transform.mat4(), transform.normalMat4() };
            resources.gbuffModelsBuffer[frameData.frameIndex].writeAndFlushAtIndex(objectId, &md, sizeof(ModelData));

            uint32_t dynamicOffset = objectId * static_cast<uint32_t>(resources.gbuffModelsBuffer[frameData.frameIndex].instanceAlignmentSize);
            {
                DUSK_PROFILE_GPU_ZONE("gbuffer_bind_model", commandBuffer);
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    resources.gbuffPipelineLayout->get(),
                    MODEL_SET_INDEX,
                    1,
                    &resources.gbuffModelDescriptorSet[frameData.frameIndex]->set,
                    1,
                    &dynamicOffset);
            }

            vkCmdPushConstants(
                commandBuffer,
                resources.gbuffPipelineLayout->get(),
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(DrawData),
                &push);

            {
                DUSK_PROFILE_GPU_ZONE("gbuffer_bind_vertex", commandBuffer);
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

                vkCmdBindIndexBuffer(commandBuffer, mesh.getIndexBuffer().vkBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            }

            {
                DUSK_PROFILE_GPU_ZONE("gbuffer_draw", commandBuffer);
                vkCmdDrawIndexed(commandBuffer, mesh.getIndexCount(), 1, 0, 0, 0);
            }
        }
    }
}
} // namespace dusk