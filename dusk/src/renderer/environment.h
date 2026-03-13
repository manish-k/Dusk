#pragma once

#include "dusk.h"

#include "renderer/gfx_buffer.h"
#include "renderer/frame_data.h"

#include <glm/glm.hpp>

namespace dusk
{
class TextureDB;
class VkGfxDevice;
class VkGfxRenderPipeline;
class VkGfxComputePipeline;
class VkGfxPipelineLayout;

struct VkGfxDescriptorPool;
struct VkGfxDescriptorSet;
struct VkGfxDescriptorSetLayout;

struct FrameData;

const glm::vec3    DEFAULT_DAY_SUN_DIRECTION   = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
const glm::vec3    DEFAULT_NIGHT_SUN_DIRECTION = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));

constexpr uint32_t ENV_RENDER_WIDTH            = 512;
constexpr uint32_t ENV_RENDER_HEIGHT           = 512;
const uint32_t     IRRADIANCE_RENDER_WIDTH     = 32;
const uint32_t     IRRADIANCE_RENDER_HEIGHT    = 32;
const uint32_t     PREFILTERED_RENDER_WIDTH    = 128;
const uint32_t     PREFILTERED_RENDER_HEIGHT   = 128;

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

struct IBLPushConstant
{
    uint32_t frameIndex;
    uint32_t envCubeMapTexId;
    uint32_t sampleCount;
    uint32_t resolution;
    float    roughness;
};

class Environment
{
public:
    Environment(TextureDB& db) :
        m_textureDB(db) { };
    ~Environment() = default;

    bool                 init(VkGfxDescriptorSetLayout& globalDescSetLayout);
    bool                 initHW(VkGfxDescriptorSetLayout& globalDescSetLayout, uint32_t maxFramesCount);
    void                 cleanup();

    bool                 areHosekWilkieParamsDirty() const { return m_hwParamsDirty; }
    void                 markHosekWilkieParamsClean() { m_hwParamsDirty = false; }

    VkGfxRenderPipeline& getPipeline() const { return *m_skyBoxRenderPipeline; }
    VkGfxPipelineLayout& getPipelineLayout() const { return *m_skyBoxRenderPipelineLayout; }
    uint32_t             getSkyTextureId() const { return m_skyTextureId; };
    uint32_t             getSkyPrefilteredTextureId() const { return m_skyPrefilteredTexId; };
    uint32_t             getSkyIrradianceTextureId() const { return m_skyIrradianceTexId; };

    void                 updateHosekWilkieSkyParams(float turbidity, float albedo, glm::vec3 sunDirection);
    void                 update(FrameData& frameData);

    bool                 needToGenerateEnvMaps() const { return m_needToGenerateEnvMaps; }
    void                 markEnvMapsGenerated() { m_needToGenerateEnvMaps = false; }

private:
    void initSkyBoxPipeline(
        std::string&              shaderPath,
        VkGfxDescriptorSetLayout& globalDescSetLayout);

    void computeHosekWilkieParams(float turbidity, float albedo, glm::vec3 sunDirection);

    void setupHWSkyResources(const std::string& shaderPath, uint32_t maxFramesCount);
    void cleanupHWSkyResources();

private:
    TextureDB&                       m_textureDB;

    HosekWilkieSkyParams             m_hwParams                        = {};
    bool                             m_hwParamsDirty                   = true;
    bool                             m_needToGenerateEnvMaps           = true;

    uint32_t                         m_skyTextureId                    = 0u;
    uint32_t                         m_skyIrradianceTexId              = 0u;
    uint32_t                         m_skyPrefilteredTexId             = 0u;

    GfxBuffer                        m_hwParamsBuffer                  = {};
    Unique<VkGfxDescriptorPool>      m_genCubeDescPool                 = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_genCubeDescLayout               = nullptr;
    Unique<VkGfxDescriptorSet>       m_genCubeDescSet                  = nullptr;

    Unique<VkGfxRenderPipeline>      m_skyBoxRenderPipeline            = nullptr;
    Unique<VkGfxPipelineLayout>      m_skyBoxRenderPipelineLayout      = nullptr;

    Unique<VkGfxComputePipeline>     m_genEnvCubeMapPipeline           = nullptr;
    Unique<VkGfxPipelineLayout>      m_genEnvCubeMapPipelineLayout     = nullptr;

    Unique<VkGfxComputePipeline>     m_genEnvIrradiancePipeline        = nullptr;
    Unique<VkGfxPipelineLayout>      m_genEnvIrradiancePipelineLayout  = nullptr;

    Unique<VkGfxComputePipeline>     m_genEnvPrefilteredPipeline       = nullptr;
    Unique<VkGfxPipelineLayout>      m_genEnvPrefilteredPipelineLayout = nullptr;
};
} // namespace dusk