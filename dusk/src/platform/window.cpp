#include "window.h"
#include "platform/glfw_vulkan_window.h"

namespace dusk
{
    Unique<Window> Window::createWindow(Properties& props)
    {
        // check render backend api and then create
        // appropriate window

        return createUnique<GLFWVulkanWindow>(props);
    }
}