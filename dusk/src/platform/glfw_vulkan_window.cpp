#include "glfw_vulkan_window.h"

namespace dusk
{
    GLFWVulkanWindow::GLFWVulkanWindow(Window::Properties& props) : m_props(props)
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
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_window = glfwCreateWindow(m_props.width, m_props.height, m_props.title.c_str(), nullptr, nullptr);

        glfwSetWindowUserPointer(m_window, this);
    }

    void GLFWVulkanWindow::onUpdate(float dt)
    {
        if (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
        }
    }
    void GLFWVulkanWindow::setEventCallback(const EventCallbackFn& cb)
    {
        m_eventCallback = cb;

        glfwSetWindowCloseCallback(m_window,
                                   [](GLFWwindow* window)
                                   {
                                       auto currentWindow =
                                           reinterpret_cast<GLFWVulkanWindow*>(glfwGetWindowUserPointer(window));

                                       WindowCloseEvent ev;
                                       currentWindow->onEvent(ev);
                                   });
    }

    void GLFWVulkanWindow::onEvent(Event& ev)
    {
        DASSERT(m_eventCallback, "Event callback has not been assigned");
        m_eventCallback(ev);
    }
} // namespace dusk