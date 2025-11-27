#pragma once

#include "dusk.h"
#include "renderer/gfx_buffer.h"

namespace dusk
{

// fwd declarations
class GlobalUbo;
class Scene;
class AmbientLightComponent;
class DirectionalLightComponent;
class PointLightComponent;
class SpotLightComponent;

struct VkGfxDescriptorPool;
struct VkGfxDescriptorSetLayout;
struct VkGfxDescriptorSet;

// need multiple of 4 for easy packing inside global ubo (uvec4[])
// but not a strict rule
inline constexpr uint32_t MAX_LIGHTS_PER_TYPE    = 128;
inline constexpr uint32_t AMBIENT_BIND_INDEX     = 0;
inline constexpr uint32_t DIRECTIONAL_BIND_INDEX = 1;
inline constexpr uint32_t POINT_BIND_INDEX       = 2;
inline constexpr uint32_t SPOT_BIND_INDEX        = 3;

class LightsSystem
{
public:
    LightsSystem();
    ~LightsSystem();

    /**
     * @brief get the reference to static instance of the system
     * @return reference to the system
     */
    static LightsSystem& get() { return *s_instance; }

    /**
     * @brief register all the lights in the scene
     * @param scene reference
     */
    void registerAllLights(Scene& scene);

    /**
     * @brief Register ambient light in the scene
     * @param ambient light component
     */
    void registerAmbientLight(AmbientLightComponent& light);

    /**
     * @brief Register a single directional light in the scene
     * @param directional light component
     */
    void registerDirectionalLight(DirectionalLightComponent& light);

    /**
     * @brief Register a single point light in the scene
     * @param point light component
     */
    void registerPointLight(PointLightComponent& light);

    /**
     * @brief Register a single spot light in the scene
     * @param spot light component
     */
    void registerSpotLight(SpotLightComponent& light);

    /**
     * @brief update all the lights data into the graphics buffer and update global ubo with count and indices
     * @param scene
     * @param ubo to update
     */
    void updateLights(Scene& scene, GlobalUbo& ubo);

    /**
     * @brief Get lights descriptor set layout
     * @return descriptor set layout
     */
    VkGfxDescriptorSetLayout& getLightsDescriptorSetLayout() const { return *m_lightsDescriptorSetLayout; };

    /**
     * @brief Get lights descriptor set
     * @return descriptor set
     */
    VkGfxDescriptorSet& getLightsDescriptorSet() const { return *m_lightsDescriptorSet; };

    uint32_t            getDirectionalLightsCount() { return m_directionalLightsCount; };
    uint32_t            getPointLightsCount() { return m_pointLightsCount; };
    uint32_t            getSpotLightsCount() { return m_spotLightsCount; };

private:
    /**
     * @brief Setup all the descriptors and buffer related resources
     */
    void setupDescriptors();

private:
    Unique<VkGfxDescriptorPool>      m_lightsDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_lightsDescriptorSetLayout = nullptr;
    Unique<VkGfxDescriptorSet>       m_lightsDescriptorSet       = nullptr;

    GfxBuffer                        m_ambientLightBuffer;
    GfxBuffer                        m_directionalLightsBuffer;
    GfxBuffer                        m_pointLightsBuffer;
    GfxBuffer                        m_spotLightsBuffer;

    uint32_t                         m_directionalLightsCount = 0u;
    uint32_t                         m_pointLightsCount       = 0u;
    uint32_t                         m_spotLightsCount        = 0u;

private:
    static LightsSystem* s_instance;
};
} // namespace dusk