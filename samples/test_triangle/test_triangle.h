#pragma once

#include "dusk.h"
#include "core/application.h"

#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"
#include "backend/vulkan/vk_renderpass.h"

using namespace dusk;

class TestTriangle final : public dusk::Application
{
public:
    TestTriangle();
    ~TestTriangle();

    bool start() override;
    void shutdown() override;
    void onUpdate(TimeStep dt) override;
    void onEvent(dusk::Event& ev) override;

private:
    Unique<VkGfxRenderPipeline> m_renderPipeline = nullptr;
    Unique<VkGfxPipelineLayout> m_pipelineLayout = nullptr;

    DynamicArray<char>          m_vertShaderCode;
    DynamicArray<char>          m_fragShaderCode;

    //VulkanRenderer*             m_renderer = nullptr;
};
