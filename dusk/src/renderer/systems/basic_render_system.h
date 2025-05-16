#pragma once

#include "dusk.h"

#include "renderer/gfx_buffer.h"

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

struct DrawData
{
    uint32_t cameraBufferIdx;
    uint32_t materialIdx;
    uint32_t directionalLightsCount;
    uint32_t pointLightsCount;
    uint32_t spotLightsCount;
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
    BasicRenderSystem(
        VkGfxDevice&              device,
        VkGfxDescriptorSetLayout& globalSet,
        VkGfxDescriptorSetLayout& materialSet);
    ~BasicRenderSystem();

    void renderGameObjects(const FrameData& frameData);

private:
    void createPipeLine();
    void createPipelineLayout(
        VkGfxDescriptorSetLayout& globalSet,
        VkGfxDescriptorSetLayout& materialSet);
    void setupDescriptors();

private:
    VkGfxDevice&                     m_device;

    Unique<VkGfxRenderPipeline>      m_renderPipeline           = nullptr;
    Unique<VkGfxPipelineLayout>      m_pipelineLayout           = nullptr;

    Unique<VkGfxDescriptorPool>      m_modelDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_modelDescriptorSetLayout = nullptr;
    Unique<VkGfxDescriptorSet>       m_modelDescriptorSet       = nullptr;

    GfxBuffer                        m_modelsBuffer;
};
} // namespace dusk