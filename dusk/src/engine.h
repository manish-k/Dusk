#pragma once

#include "dusk.h"
#include "core/dtime.h"
#include "core/application.h"
#include "events/event.h"
#include "platform/window.h"
#include "scene/scene.h"
#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_renderer.h"
#include "backend/vulkan/vk_descriptors.h"
#include "renderer/render_api.h"
#include "renderer/systems/basic_render_system.h"
#include "renderer/systems/grid_render_system.h"
#include "ui/ui.h"

namespace dusk
{
const uint32_t maxMaterialCount = 1000;

class Engine final
{
public:
    struct Config
    {
        RenderAPI::API renderAPI;

        static Config  defaultConfig()
        {
            auto config      = Config {};
            config.renderAPI = RenderAPI::API::VULKAN;
            return config;
        }
    };

public:
    Engine(const Config& conf);
    ~Engine();

    CLASS_UNCOPYABLE(Engine)

    bool                  start(Shared<Application> app);
    void                  run();
    void                  stop();
    void                  shutdown();
    void                  onUpdate(TimeStep dt);
    void                  onEvent(Event& ev);
    void                  loadScene(Scene* scene);

    static Engine&        get() { return *s_instance; }
    static RenderAPI::API getRenderAPI() { return s_instance->m_config.renderAPI; }

    VulkanRenderer&       getRenderer() { return *m_renderer; }
    VkGfxDevice&          getGfxDevice() { return *m_gfxDevice; }

    bool                  setupGlobals();
    void                  cleanupGlobals();

    void                  registerTextures(DynamicArray<Texture>& textures);
    void                  registerMaterials(DynamicArray<Material>& materials);

private:
    Config                           m_config;

    Unique<VkGfxDevice>              m_gfxDevice         = nullptr;
    Unique<VulkanRenderer>           m_renderer          = nullptr;

    Shared<Window>                   m_window            = nullptr;
    Shared<Application>              m_app               = nullptr;

    Unique<BasicRenderSystem>        m_basicRenderSystem = nullptr;
    Unique<GridRenderSystem>         m_gridRenderSystem  = nullptr;

    Unique<UI>                       m_ui                = nullptr;

    bool                             m_running           = false;
    bool                             m_paused            = false;

    Scene*                           m_currentScene      = nullptr;

    TimePoint                        m_lastFrameTime;
    TimeStep                         m_deltaTime;

    Unique<VkGfxDescriptorPool>      m_globalDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_globalDescriptorSetLayout = nullptr;

    VulkanGfxBuffer                  m_globalUbos;
    Unique<VkGfxDescriptorSet>       m_globalDescriptorSet;

    Unique<VkGfxDescriptorPool>      m_materialDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_materialDescriptorSetLayout = nullptr;

    VulkanGfxBuffer                  m_materialsBuffer;
    Unique<VkGfxDescriptorSet>       m_materialsDescriptorSet;

private:
    static Engine* s_instance;
};
} // namespace dusk