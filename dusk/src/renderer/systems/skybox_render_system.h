#pragma once

#include "dusk.h"

#include "renderer/sub_mesh.h"
#include "renderer/texture.h"

namespace dusk
{
class Scene;
class VkGfxDevice;
class VkGfxRenderPipeline;
class VkGfxPipelineLayout;

struct VkGfxDescriptorPool;
struct VkGfxDescriptorSetLayout;
struct VkGfxDescriptorSet;
struct FrameData;

struct SkyboxData
{
    uint32_t cameraBufferIdx;
};

class SkyboxRenderSystem
{
public:
    SkyboxRenderSystem(
        VkGfxDevice&              device,
        VkGfxDescriptorSetLayout& globalSetLayout);
    ~SkyboxRenderSystem();

    void renderSkybox(const FrameData& frameData);

    void setVisble(bool visibility);

private:
    void createPipeLine();
    void createPipelineLayout(VkGfxDescriptorSetLayout& globalSetLayout);
    void setupDescriptors();

private:
    VkGfxDevice&                     m_device;
    bool                             m_isEnable               = true;

    Unique<VkGfxRenderPipeline>      m_renderPipeline         = nullptr;
    Unique<VkGfxPipelineLayout>      m_pipelineLayout         = nullptr;

    Unique<VkGfxDescriptorPool>      m_texDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_texDescriptorSetLayout = nullptr;
    Unique<VkGfxDescriptorSet>       m_texDescriptorSet       = nullptr;

    SubMesh                          m_cubeMesh               = {};
    Texture3D                        m_skyboxTexture          = { 999 }; // TODO:: for now some big enough id is given, need a failsafe id generation
};
} // namespace dusk