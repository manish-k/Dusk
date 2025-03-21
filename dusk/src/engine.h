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

namespace dusk
{
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

private:
    Config                           m_config;

    Unique<VkGfxDevice>              m_gfxDevice         = nullptr;
    Unique<VulkanRenderer>           m_renderer          = nullptr;

    Shared<Window>                   m_window            = nullptr;
    Shared<Application>              m_app               = nullptr;

    Unique<BasicRenderSystem>        m_basicRenderSystem = nullptr;

    bool                             m_running           = false;
    bool                             m_paused            = false;

    Scene*                           m_currentScene      = nullptr;

    TimePoint                        m_lastFrameTime;
    TimeStep                         m_deltaTime;

    Unique<VkGfxDescriptorPool>      m_globalDescriptorPool       = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_globalDescritptorSetLayout = nullptr;
    DynamicArray<VulkanGfxBuffer>    m_globalUbos {};
    DynamicArray<VkGfxDescriptorSet> m_globalDescriptorSets {};

private:
    static Engine* s_instance;
};
} // namespace dusk