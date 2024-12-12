#pragma once

#include "dusk.h"
#include "backend/vulkan/vk.h"
#include "window.h"
#include "core/key_codes.h"
#include "events/app_event.h"
#include "events/key_event.h"
#include "events/mouse_event.h"

#include <volk.h>
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include <string>

namespace dusk
{
/**
 * @brief Window class for the GLFW vulkan implementation
 */
class GLFWVulkanWindow final : public Window
{
public:
    GLFWVulkanWindow(Window::Properties& props);
    ~GLFWVulkanWindow() override;

    CLASS_UNCOPYABLE(GLFWVulkanWindow)

    void     onUpdate(TimeStep dt) override;
    void     onEvent(Event& ev);

    uint32_t getHeight() const override { return m_props.height; }
    uint32_t getWidth() const override { return m_props.width; }
    bool     isResized() const override { return m_isResized; }
    void*    getNativeWindow() const override { return (void*)m_window; };

    void     toggleFullScreen() override;
    void     toggleFullScreenBorderless() override;

    /**
     * @brief Set new width of the current window
     * @param newWidth
     */
    void setWidth(uint32_t newWidth)
    {
        m_props.width = newWidth;
        m_isResized   = true;
    }

    /**
     * @brief Set new height of the current window
     * @param newHeight
     */
    void setHeight(uint32_t newHeight)
    {
        m_props.height = newHeight;
        m_isResized    = true;
    }

    /**
     * @brief Reset the resized flag for the window
     */
    void resetResizedState() { m_isResized = false; }

    /**
     * @brief Set current cursor x position in the window relative to top-left corner
     * @param newPosX
     */
    void setCursorPosX(int newPosX) { m_cursorPosX = newPosX; }

    /**
     * @brief Set current cursor y position in the window relative to top-left corner
     * @param newPosY
     */
    void                      setCursorPosy(int newPosY) { m_cursorPosY = newPosY; }

    void                      setEventCallback(const EventCallbackFn& cb) override;

    DynamicArray<const char*> getRequiredWindowExtensions();

    Error                     createWindowSurface(VkInstance instance, VkSurfaceKHR* pSurface);

private:
    /**
     * @brief Initialize GLFW library
     * @return True if successful else false
     */
    bool initWindow();

    /**
     * @brief Create window based on the property given during initialization
     */
    void createWindow();

private:
    GLFWwindow*        m_window;
    Window::Properties m_props;

    EventCallbackFn    m_eventCallback = nullptr;

    int                m_windowPosX;
    int                m_windowPosY;

    bool               m_isResized = false;

    int                m_cursorPosX;
    int                m_cursorPosY;
};
} // namespace dusk