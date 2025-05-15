#pragma once

#include "dusk.h"
#include "renderer/gfx_buffer.h"
#include "backend/vulkan/vk_descriptors.h"

namespace dusk
{
class GlobalUbo;
class Scene;
class AmbientLightComponent;
class DirectionalLightComponent;
class PointLightComponent;
class SpotLightComponent;

inline constexpr uint32_t MAX_LIGHTS_PER_TYPE    = 128; // need multiple of 4 for easy packing for global ubo
inline constexpr uint32_t AMBIENT_BIND_INDEX     = 0;
inline constexpr uint32_t DIRECTIONAL_BIND_INDEX = 1;
inline constexpr uint32_t POINT_BIND_INDEX       = 2;
inline constexpr uint32_t SPOT_BIND_INDEX        = 3;

class LightsSystem
{
public:
    LightsSystem();
    ~LightsSystem();

    static LightsSystem&      get() { return *s_instance; }

    void                      registerAllLights(Scene& scene);
    void                      registerAmbientLight(AmbientLightComponent& light);
    void                      registerDirectionalLight(DirectionalLightComponent& light);
    void                      registerPointLight(PointLightComponent& light);
    void                      registerSpotLight(SpotLightComponent& light);
    
    void                      updateLights(Scene& scene, GlobalUbo& ubo);

    VkGfxDescriptorSetLayout& getLightsDescriptorSetLayout() const { return *m_lightsDescriptorSetLayout; };
    VkGfxDescriptorSet&       getLightsDescriptorSet() const { return *m_lightsDescriptorSet; };

private:
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