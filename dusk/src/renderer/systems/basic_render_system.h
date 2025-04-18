#pragma once

#include "scene/scene.h"
#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"
#include "backend/vulkan/vk_descriptors.h"
#include "renderer/frame_data.h"

namespace dusk
{
struct BasicPushConstantsData
{
    glm::mat4 model { 1.f };
    glm::mat4 normal { 1.f };
};

struct DrawData
{
    uint32_t cameraIdx;
    uint32_t modelIdx;
    uint32_t materialIdx;
};

struct ModelData
{
    glm::mat4 model { 1.f };
    glm::mat4 normal { 1.f };
};

// TODO: figure out the best way to configure this
constexpr uint32_t maxRenderableMeshes = 10000;

class BasicRenderSystem
{
public:
    BasicRenderSystem(VkGfxDevice& device, VkGfxDescriptorSetLayout& globalSetLayout);
    ~BasicRenderSystem();

    void renderGameObjects(const FrameData& frameData);

private:
    void createPipeLine();
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void setupDescriptors();

private:
    VkGfxDevice&                     m_device;

    Unique<VkGfxRenderPipeline>      m_renderPipeline           = nullptr;
    Unique<VkGfxPipelineLayout>      m_pipelineLayout           = nullptr;

    Unique<VkGfxDescriptorPool>      m_modelDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_modelDescriptorSetLayout = nullptr;
    Unique<VkGfxDescriptorSet>       m_modelDescriptorSet       = nullptr;

    VulkanGfxBuffer                  m_modelsBuffer;
};
} // namespace dusk