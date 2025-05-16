#pragma once

#include "dusk.h"

namespace dusk
{
// fwd declarations
class Scene;
class VkGfxDevice;
class VkGfxRenderPipeline;
class VkGfxPipelineLayout;

struct VkGfxDescriptorPool;
struct VkGfxDescriptorSetLayout;
struct VkGfxDescriptorSet;
struct FrameData;

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