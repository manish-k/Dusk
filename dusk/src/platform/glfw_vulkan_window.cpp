#include "glfw_vulkan_window.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace dusk
{
void gLFWErrorCallback(int errorCode, const char* description)
{
    DUSK_ERROR("GLFW error code{}: {}", errorCode, description);
}

GLFWVulkanWindow::GLFWVulkanWindow(Window::Properties& props) :
    m_props(props)
{
    if (initWindow())
    {
        createWindow();
    }
    else
    {
        DUSK_CRITICAL("Unable to initialize GLFW Window");
        m_window = nullptr;
    }
}

GLFWVulkanWindow::~GLFWVulkanWindow()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool GLFWVulkanWindow::initWindow()
{
    if (!glfwInit())
    {
        return false;
    }
    return true;
}

void GLFWVulkanWindow::createWindow()
{
    glfwSetErrorCallback(gLFWErrorCallback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, int(m_props.resizable));

    auto*      monitor = glfwGetPrimaryMonitor();
    const auto mode    = glfwGetVideoMode(monitor);

    switch (m_props.mode)
    {
        case Window::Mode::Fullscreen:
        {
            m_window = glfwCreateWindow(mode->width, mode->height, m_props.title.c_str(), monitor, nullptr);
            break;
        }

        case Window::Mode::FullscreenBorderless:
        {
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            m_window = glfwCreateWindow(mode->width, mode->height, m_props.title.c_str(), monitor, nullptr);
            break;
        }

        case Window::Mode::Maximized:
        {
            glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        }

        default:
            m_window = glfwCreateWindow(m_props.width, m_props.height, m_props.title.c_str(), nullptr, nullptr);
    }

    glfwGetWindowPos(m_window, &m_windowPosX, &m_windowPosY);
    glfwSetWindowUserPointer(m_window, this);
}

void GLFWVulkanWindow::onUpdate(TimeStep dt)
{
    if (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
    }
}

void GLFWVulkanWindow::onEvent(Event& ev)
{
    DASSERT(m_eventCallback, "Event callback has not been assigned");
    m_eventCallback(ev);
}

void GLFWVulkanWindow::setEventCallback(const EventCallbackFn& cb)
{
    m_eventCallback = cb;

    // window close event
    glfwSetWindowCloseCallback(m_window,
                               [](GLFWwindow* window)
                               {
                                   auto             currentWindow = reinterpret_cast<GLFWVulkanWindow*>(glfwGetWindowUserPointer(window));

                                   WindowCloseEvent ev;
                                   currentWindow->onEvent(ev);
                               });

    // window resize event
    glfwSetFramebufferSizeCallback(m_window,
                                   [](GLFWwindow* window, int width, int height)
                                   {
                                       auto currentWindow = reinterpret_cast<GLFWVulkanWindow*>(glfwGetWindowUserPointer(window));

                                       currentWindow->setWidth(width);
                                       currentWindow->setHeight(height);

                                       WindowResizeEvent ev(width, height);
                                       currentWindow->onEvent(ev);
                                   });

    // window minimized/maximized event
    glfwSetWindowIconifyCallback(m_window,
                                 [](GLFWwindow* window, int iconified)
                                 {
                                     auto                 currentWindow = reinterpret_cast<GLFWVulkanWindow*>(glfwGetWindowUserPointer(window));

                                     WindowIconifiedEvent ev(iconified);
                                     currentWindow->onEvent(ev);
                                 });

    // keyboard event
    glfwSetKeyCallback(m_window,
                       [](GLFWwindow* window, int key, int scancode, int action, int mods)
                       {
                           auto currentWindow = reinterpret_cast<GLFWVulkanWindow*>(glfwGetWindowUserPointer(window));

                           switch (action)
                           {
                               case GLFW_PRESS:
                               {
                                   auto            keyCode = KeyCode(key);
                                   KeyPressedEvent ev(keyCode);

                                   currentWindow->onEvent(ev);
                                   break;
                               }

                               case GLFW_RELEASE:
                               {
                                   auto             keyCode = KeyCode(key);
                                   KeyReleasedEvent ev(keyCode);

                                   currentWindow->onEvent(ev);
                                   break;
                               }

                               case GLFW_REPEAT:
                               {
                                   auto           keyCode = KeyCode(key);
                                   KeyRepeatEvent ev(keyCode);

                                   currentWindow->onEvent(ev);
                                   break;
                               }

                               default:
                                   DUSK_WARN("Unidentified Key input action received {}", key);
                           }
                       });

    // mouse button pressed
    glfwSetMouseButtonCallback(m_window,
                               [](GLFWwindow* window, int button, int action, int mods)
                               {
                                   auto currentWindow = reinterpret_cast<GLFWVulkanWindow*>(glfwGetWindowUserPointer(window));

                                   switch (action)
                                   {
                                       case GLFW_PRESS:
                                       {
                                           auto                    btnCode = MouseCode(button);
                                           double x, y;
                                           glfwGetCursorPos(window, &x, &y);
                                           MouseButtonPressedEvent ev(btnCode, x, y);

                                           currentWindow->onEvent(ev);
                                           break;
                                       }

                                       case GLFW_RELEASE:
                                       {
                                           auto                     btnCode = MouseCode(button);
                                           double x, y;
                                           glfwGetCursorPos(window, &x, &y);
                                           MouseButtonReleasedEvent ev(btnCode, x, y);

                                           currentWindow->onEvent(ev);
                                           break;
                                       }

                                       default:
                                           DUSK_WARN("Unidentified mouse btn input action received {}", button);
                                   }
                               });

    // mouse scroll event
    glfwSetScrollCallback(m_window,
                          [](GLFWwindow* window, double xoffset, double yoffset)
                          {
                              auto               currentWindow = reinterpret_cast<GLFWVulkanWindow*>(glfwGetWindowUserPointer(window));

                              MouseScrolledEvent ev(xoffset, yoffset);

                              currentWindow->onEvent(ev);
                          });

    // cursor position change event
    // coordinates obtained are relative to top-left corner of the window
    glfwSetCursorPosCallback(m_window,
                             [](GLFWwindow* window, double xpos, double ypos)
                             {
                                 auto currentWindow = reinterpret_cast<GLFWVulkanWindow*>(glfwGetWindowUserPointer(window));

                                 currentWindow->setCursorPosX(xpos);
                                 currentWindow->setCursorPosy(ypos);

                                 MouseMovedEvent ev(xpos, ypos);

                                 currentWindow->onEvent(ev);
                             });
}

void GLFWVulkanWindow::toggleFullScreen()
{
    auto               monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

    if (m_props.mode == Window::Mode::Fullscreen)
    {
        // change to default
        glfwSetWindowMonitor(m_window, nullptr, m_windowPosX, m_windowPosY, m_props.width, m_props.height, mode->refreshRate);
        m_props.mode = Window::Mode::Default;
    }
    else
    {
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        m_props.mode = Window::Mode::Fullscreen;
    }
}

void GLFWVulkanWindow::toggleFullScreenBorderless()
{
    auto               monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

    if (m_props.mode == Window::Mode::FullscreenBorderless)
    {
        // change to default
        glfwSetWindowMonitor(m_window, nullptr, m_windowPosX, m_windowPosY, m_props.width, m_props.height, mode->refreshRate);
        m_props.mode = Window::Mode::Default;
    }
    else
    {
        // TODO: currently same as fullscreen window, need to fix
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        m_props.mode = Window::Mode::FullscreenBorderless;
    }
}

DynamicArray<const char*> GLFWVulkanWindow::getRequiredWindowExtensions()
{
    uint32_t                  extensionCount = 0;
    const char**              glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    DynamicArray<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);

    return extensions;
}

VkExtent2D GLFWVulkanWindow::getFramebufferExtent()
{
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    return actualExtent;
}

Error GLFWVulkanWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* pSurface)
{
    VulkanResult result = glfwCreateWindowSurface(instance, m_window, nullptr, pSurface);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create window surface {}", result.toString());
    }
    DUSK_INFO("Window surface created successfully");

    return result.getErrorId();
}
} // namespace dusk