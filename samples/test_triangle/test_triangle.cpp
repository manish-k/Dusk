#include "test_triangle.h"

#include "core/entrypoint.h"
#include "scene/scene.h"
#include "events/key_event.h"
#include "events/mouse_event.h"
#include "platform/file_system.h"
#include "backend/vulkan/vk_renderer.h"

TestTriangle::TestTriangle() { }

TestTriangle::~TestTriangle() { APP_INFO("Destroying Test Application"); }

Unique<dusk::Application> dusk::createApplication(int argc, char** argv)
{
    APP_INFO("Creating Test Application");
    return createUnique<TestTriangle>();
}

bool TestTriangle::start()
{
    VulkanContext&  vkContext = VulkanRenderer::getVulkanContext();
    Engine&         engine    = Engine::get();
    VulkanRenderer* renderer  = static_cast<VulkanRenderer*>(engine.getRenderer());
    VkGfxSwapChain& swapChain = renderer->getSwapChain();

    // load shaders
    m_vertShaderCode = FileSystem::readFileBinary("assets/shaders/simple.vert.spv");

    m_fragShaderCode = FileSystem::readFileBinary("assets/shaders/simple.frag.spv");

    m_renderPass     = VkGfxRenderPass::Builder(vkContext).setColorAttachmentFormat(swapChain.getImageFormat()).build();
    swapChain.initFrameBuffers(*m_renderPass);

    m_pipelineLayout = VkGfxPipelineLayout::Builder(vkContext).build();

    // create pipeline
    m_renderPipeline = VkGfxRenderPipeline::Builder(vkContext)
                           .setVertexShaderCode(m_vertShaderCode)
                           .setFragmentShaderCode(m_fragShaderCode)
                           .setPipelineLayout(*m_pipelineLayout)
                           .setRenderPass(*m_renderPass)
                           .build();

    return true;
}

void TestTriangle::shutdown() { }

void TestTriangle::onUpdate(TimeStep dt)
{
    Engine&         engine        = Engine::get();
    VulkanRenderer* renderer      = static_cast<VulkanRenderer*>(engine.getRenderer());
    VulkanContext&  vkContext     = VulkanRenderer::getVulkanContext();

    auto            commandBuffer = renderer->beginFrame();

    renderer->beginSwapChainRenderPass(commandBuffer, m_renderPass->get());

    m_renderPipeline->bind(commandBuffer);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    renderer->endSwapChainRenderPass(commandBuffer);

    renderer->endFrame();

    vkDeviceWaitIdle(vkContext.device);
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
