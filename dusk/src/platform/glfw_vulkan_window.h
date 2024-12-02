#pragma once

#include "dusk.h"
#include "window.h"
#include "core/key_codes.h"
#include "events/app_event.h"
#include "events/key_event.h"
#include "events/mouse_event.h"

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include <string>

namespace dusk
{
    class GLFWVulkanWindow : public Window
    {
    public:
        GLFWVulkanWindow(Window::Properties& props);
        ~GLFWVulkanWindow() override;

        void onUpdate(float dt) override;
        void onEvent(Event& ev);

        uint32_t getHeight() const override { return m_props.height; }
        uint32_t getWidth() const override { return m_props.width; }
        bool isResized() const override { return m_isResized; }

        void toggleFullScreen() override;
        void toggleFullScreenBorderless() override;

        void setWidth(uint32_t newWidth) { m_props.width = newWidth; }
        void setHeight(uint32_t newHeight) { m_props.height = newHeight; }
        void setResized(bool newState) { m_isResized = newState; }
        void resetResizedState() { m_isResized = false; }
        void setEventCallback(const EventCallbackFn& cb);

        void* getNativeWindow() const override { return (void*)m_window; };

    private:
        bool initWindow();
        void createWindow();

    private:
        GLFWwindow* m_window;
        Window::Properties m_props;

        EventCallbackFn m_eventCallback = nullptr;

        int m_windowPosX;
        int m_windowPosY;

        bool m_isResized = false;
    };
} // namespace dusk