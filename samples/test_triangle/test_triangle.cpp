#include "test_triangle.h"

#include "core/entrypoint.h"
#include "scene/scene.h"
#include "events/key_event.h"
#include "events/mouse_event.h"
#include "platform/file_system.h"
#include "backend/vulkan/vk_renderer.h"
#include "backend/vulkan/vk_debug.h"

TestTriangle::TestTriangle() { }

TestTriangle::~TestTriangle() { APP_INFO("Destroying Test Application"); }

Unique<dusk::Application> dusk::createApplication(int argc, char** argv)
{
    APP_INFO("Creating Test Application");
    return createUnique<TestTriangle>();
}

bool TestTriangle::start()
{
    VulkanContext&  vkContext = VkGfxDevice::getSharedVulkanContext();
    Engine&         engine    = Engine::get();
    VulkanRenderer& renderer  = engine.getRenderer();
    VkGfxSwapChain& swapChain = renderer.getSwapChain();

    // load shaders
    m_vertShaderCode = FileSystem::readFileBinary("assets/shaders/simple.vert.spv");

    m_fragShaderCode = FileSystem::readFileBinary("assets/shaders/simple.frag.spv");

    m_pipelineLayout = VkGfxPipelineLayout::Builder(vkContext).build();

    // create pipeline
    m_renderPipeline = VkGfxRenderPipeline::Builder(vkContext)
                           .setVertexShaderCode(m_vertShaderCode)
                           .setFragmentShaderCode(m_fragShaderCode)
                           .setPipelineLayout(*m_pipelineLayout)
                           .addColorAttachmentFormat(swapChain.getImageFormat())
                           .setDebugName("test_triangle_pipeline")
                           .build();

    return true;
}

void TestTriangle::shutdown()
{
    // TODO: Ideally destruction of graphics objects should not be handled by Applications.
    // I think gfxdevice should track allocated all gfx objects and should handle order of
    // destruction of gfx objects and vulkan objects.
    // For now setting unique ptr to nullptr can trigger destruction

    m_renderPipeline = nullptr;
    m_pipelineLayout = nullptr;
}

void TestTriangle::onUpdate(TimeStep dt)
{
    Engine&         engine        = Engine::get();
    VulkanRenderer& renderer      = engine.getRenderer();
    VulkanContext&  vkContext     = VkGfxDevice::getSharedVulkanContext();

    auto            commandBuffer = renderer.beginFrame();

    renderer.beginRendering(commandBuffer);

    m_renderPipeline->bind(commandBuffer);

    vkdebug::cmdBeginLabel(commandBuffer, "draw", glm::vec4(0.0f, 0.8f, 0.0f, 1.0f));
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkdebug::cmdEndLabel(commandBuffer);

    renderer.endRendering(commandBuffer);

    renderer.endFrame();

    renderer.deviceWaitIdle();
}

void TestTriangle::onEvent(Event& ev)
{
    EventDispatcher dispatcher(ev);

    /*dispatcher.dispatch<KeyPressedEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<KeyReleasedEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<KeyRepeatEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<MouseButtonReleasedEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<MouseScrolledEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<MouseMovedEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });*/
}
