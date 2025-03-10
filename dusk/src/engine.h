#pragma once

#include "dusk.h"
#include "core/dtime.h"
#include "core/application.h"
#include "events/event.h"
#include "platform/window.h"
#include "renderer/render_api.h"
#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_renderer.h"

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
    Engine(Config& conf);
    ~Engine();

    CLASS_UNCOPYABLE(Engine)

    bool                  start(Shared<Application> app);
    void                  run();
    void                  stop();
    void                  shutdown();
    void                  onUpdate(TimeStep dt);
    void                  onEvent(Event& ev);

    static Engine&        get() { return *s_instance; }
    static RenderAPI::API getRenderAPI() { return s_instance->m_config.renderAPI; }

    VulkanRenderer&       getRenderer() { return *m_renderer; }
    VkGfxDevice&          getGfxDevice() { return *m_gfxDevice; }

private:
    Config                 m_config;

    Unique<VkGfxDevice>    m_gfxDevice = nullptr;
    Unique<VulkanRenderer> m_renderer  = nullptr;

    Shared<Window>         m_window    = nullptr;
    Shared<Application>    m_app       = nullptr;

    bool                   m_running   = false;
    bool                   m_paused    = false;

    TimePoint              m_lastFrameTime;
    TimeStep               m_deltaTime;

private:
    static Engine* s_instance;
};
} // namespace dusk