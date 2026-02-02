#pragma once

#include "dusk.h"

namespace dusk
{
class TextureDB;
class VkGfxDevice;
class VkGfxRenderPipeline;
class VkGfxPipelineLayout;

struct VkGfxDescriptorSetLayout;

class Environment
{
public:
    Environment(TextureDB& db) :
        m_textureDB(db) { };
    ~Environment() = default;

    bool                 init(VkGfxDescriptorSetLayout& globalDescSetLayout);
    void                 cleanup();

    VkGfxRenderPipeline& getPipeline() { return *m_skyBoxPipeline; }
    VkGfxPipelineLayout& getPipelineLayout() { return *m_skyBoxPipelineLayout; }
    uint32_t             getSkyTextureId() const { return m_skyTextureId; };
    uint32_t             getSkyPrefilteredTextureId() const { return m_skyPrefilteredTexId; };
    uint32_t             getSkyPrefilteredMaxLods() const { return m_skyPrefilteredMaxLods; };
    uint32_t             getSkyIrradianceTextureId() const { return m_skyIrradianceTexId; };

private:
    void initCubeTextureResources(
        std::string&              shaderPath,
        std::string&              resPath,
        VkGfxDescriptorSetLayout& globalDescSetLayout);

private:
    TextureDB&                  m_textureDB;

    uint32_t                    m_skyTextureId          = {};
    uint32_t                    m_skyIrradianceTexId    = {};
    uint32_t                    m_skyPrefilteredTexId   = {};
    uint32_t                    m_skyPrefilteredMaxLods = {};

    Unique<VkGfxRenderPipeline> m_skyBoxPipeline        = nullptr;
    Unique<VkGfxPipelineLayout> m_skyBoxPipelineLayout  = nullptr;
};
} // namespace dusk