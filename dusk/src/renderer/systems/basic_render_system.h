#pragma once

#include "scene/scene.h"
#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"
#include "renderer/frame_data.h"

namespace dusk
{
struct BasicPushConstantsData
{
    glm::mat4 mvpMatrix { 1.f };
};

class BasicRenderSystem
{
public:
    BasicRenderSystem(VkGfxDevice device);
    ~BasicRenderSystem();

    void renderGameObjects(const FrameData& frameData);

private:
    void createPipeLine();
    void createPipelineLayout();

private:
    VkGfxDevice&                m_device;

    Unique<VkGfxRenderPipeline> m_renderPipeline = nullptr;
    Unique<VkGfxPipelineLayout> m_pipelineLayout = nullptr;
};
} // namespace dusk