#pragma once

#include "dusk.h"

#include <glm/glm.hpp>

namespace dusk
{
class TextureDB;
class VkGfxDevice;
class VkGfxRenderPipeline;
class VkGfxPipelineLayout;

struct VkGfxDescriptorSetLayout;

const glm::vec3 DEFAULT_DAY_SUN_DIRECTION   = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
const glm::vec3 DEFAULT_NIGHT_SUN_DIRECTION = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));

struct alignas(16) HosekWilkieSkyParams
{
    glm::vec3 A = {};
    glm::vec3 B = {};
    glm::vec3 C = {};
    glm::vec3 D = {};
    glm::vec3 E = {};
    glm::vec3 F = {};
    glm::vec3 G = {};
    glm::vec3 H = {};
    glm::vec3 I = {};

    // Additional parameters for sun and sky
    glm::vec3 zenithColor  = {};
    glm::vec4 sunDirection = {}; // w component is for sun disk radius
};

class Environment
{
public:
    Environment(TextureDB& db) :
        m_textureDB(db) { };
    ~Environment() = default;

    bool                 init(VkGfxDescriptorSetLayout& globalDescSetLayout);
    void                 cleanup();

    bool                 areHosekWilkieParamsDirty() const { return m_hwParamsDirty; }
    void                 markHosekWilkieParamsClean() { m_hwParamsDirty = false; }

    VkGfxRenderPipeline& getPipeline() { return *m_skyBoxRenderPipeline; }
    VkGfxPipelineLayout& getPipelineLayout() { return *m_skyBoxRenderPipelineLayout; }
    uint32_t             getSkyTextureId() const { return m_skyTextureId; };
    uint32_t             getSkyPrefilteredTextureId() const { return m_skyPrefilteredTexId; };
    uint32_t             getSkyIrradianceTextureId() const { return m_skyIrradianceTexId; };

private:
    void initCubeTextureResources(
        std::string&              shaderPath,
        std::string&              resPath,
        VkGfxDescriptorSetLayout& globalDescSetLayout);

    void computeHosekWilkieParams(float turbidity, float albedo, glm::vec3 sunDirection);

private:
    TextureDB&                  m_textureDB;

    HosekWilkieSkyParams        m_hwParams                        = {};
    bool                        m_hwParamsDirty                   = true;

    uint32_t                    m_skyTextureId                    = 0u;
    uint32_t                    m_skyIrradianceTexId              = 0u;
    uint32_t                    m_skyPrefilteredTexId             = 0u;

    Unique<VkGfxRenderPipeline> m_skyBoxRenderPipeline            = nullptr;
    Unique<VkGfxPipelineLayout> m_skyBoxRenderPipelineLayout      = nullptr;

    Unique<VkGfxRenderPipeline> m_genEnvCubeMapPipeline           = nullptr;
    Unique<VkGfxPipelineLayout> m_genEnvCubeMapPipelineLayout     = nullptr;

    Unique<VkGfxRenderPipeline> m_genEnvIrradiancePipeline        = nullptr;
    Unique<VkGfxPipelineLayout> m_genEnvIrradiancePipelineLayout  = nullptr;

    Unique<VkGfxRenderPipeline> m_genEnvPrefilteredPipeline       = nullptr;
    Unique<VkGfxPipelineLayout> m_genEnvPrefilteredPipelineLayout = nullptr;
};
} // namespace dusk