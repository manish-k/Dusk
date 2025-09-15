#pragma once

#include "dusk.h"

namespace dusk
{
class TextureDB;
class SubMesh;
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
    SubMesh&             getCubeMesh() const { return *m_cubeMesh; };
    uint32_t             getSkyTextureId() const { return m_skyTextureId; };

private:
    TextureDB&                  m_textureDB;

    uint32_t                    m_skyTextureId         = {};
    Shared<SubMesh>             m_cubeMesh             = nullptr;

    Unique<VkGfxRenderPipeline> m_skyBoxPipeline       = nullptr;
    Unique<VkGfxPipelineLayout> m_skyBoxPipelineLayout = nullptr;
};
} // namespace dusk