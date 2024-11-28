#pragma once

#include "dusk.h"
#include "window.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include <string>

namespace dusk
{
    class GLFWVulkanWindow : public Window
    {
    public:
        GLFWVulkanWindow(const Window::Properties& props);
        virtual ~GLFWVulkanWindow() override;

        void onUpdate() override;

        uint32_t getHeight() const override { return m_props.height; }
        uint32_t getWidth() const override { return m_props.width; }

        void setEventCallback(const EventCallbackFn& cb);

        void* getNativeWindow() const override { return (void*)m_window; };

    private:
        bool initWindow();
        void createWindow();

    private:
        GLFWwindow* m_window;   
        Window::Properties m_props;
    };
} // namespace dusk