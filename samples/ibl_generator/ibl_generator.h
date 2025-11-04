#pragma once

#include "dusk.h"
#include "core/application.h"
#include "scene/scene.h"

#include "backend/vulkan/vk_descriptors.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"

using namespace dusk;

/*
Application to convert hdr images to cubemaps and also generate
irradiance and pre-filtered maps for diffuse and specular ibl.
*/

struct CubeMapPushConstant
{
    uint32_t equiRectTextureId = 0;
};

struct CubeProjView
{
    glm::mat4 projView;
};

class IBLGenerator : public Application
{
public:
    IBLGenerator();
    ~IBLGenerator();

    bool start() override;
    void shutdown() override;
    void onUpdate(TimeStep dt) override;
    void onEvent(dusk::Event& ev) override;

private:
    void setupHDRToCubeMapPipeline();
    void setupIrradiancePipeline();
    void setupPrefilteredPipeline();

    void executePipelines();

private:
    uint32_t m_hdrEnvTextureId = {};
    bool     m_executedOnce    = false;

    // hdr to cubemap
    GfxBuffer                        m_cubeProjViewBuffer         = {};
    Unique<VkGfxDescriptorPool>      m_cubeProjViewDescPool       = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_cubeProjViewDescLayout     = nullptr;
    Unique<VkGfxDescriptorSet>       m_cubeProjViewDescSet        = nullptr;

    Unique<VkGfxRenderPipeline>      m_hdrToCubeMapPipeline       = nullptr;
    Unique<VkGfxPipelineLayout>      m_hdrToCubeMapPipelineLayout = nullptr;
    uint32_t                         m_hdrCubeMapTextureId        = {};
};