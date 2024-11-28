#include "glfw_vulkan_window.h"

namespace dusk
{
    GLFWVulkanWindow::GLFWVulkanWindow(const Window::Properties& props) : m_props(props)
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

    void GLFWVulkanWindow::onUpdate()
    {
        if (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
        }
    }
    void GLFWVulkanWindow::setEventCallback(const EventCallbackFn& cb) {}
} // namespace dusk