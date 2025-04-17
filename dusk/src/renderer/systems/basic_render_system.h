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

    Unique<VkGfxRenderPipeline>      m_renderPipeline            = nullptr;
    Unique<VkGfxPipelineLayout>      m_pipelineLayout            = nullptr;

    Unique<VkGfxDescriptorPool>      m_modelDescriptorPool       = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_modelDescritptorSetLayout = nullptr;

    VulkanGfxBuffer                  m_modelsBuffer;
};
} // namespace dusk