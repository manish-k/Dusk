#pragma once

#include "scene/scene.h"
#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"
#include "backend/vulkan/vk_descriptors.h"
#include "renderer/frame_data.h"

namespace dusk
{
class GridRenderSystem
{
public:
    struct GridData
    {
        uint32_t cameraBufferIdx;
    };

public:
    GridRenderSystem(
        VkGfxDevice&              device,
        VkGfxDescriptorSetLayout& globalSetLayout);
    ~GridRenderSystem();

    void renderGrid(const FrameData& frameData);

private:
    VkGfxDevice&                m_device;

    Unique<VkGfxRenderPipeline> m_renderPipeline = nullptr;
    Unique<VkGfxPipelineLayout> m_pipelineLayout = nullptr;
};
} // namespace dusk