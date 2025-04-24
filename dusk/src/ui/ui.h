#pragma once

#include "dusk.h"
#include "platform/window.h"
#include "backend/vulkan/vk_descriptors.h"

#include <volk.h>

namespace dusk
{
class UI
{
public:
    UI()  = default;
    ~UI() = default;

    bool init(Window& window);
    void shutdown();
    void beginRendering();
    void endRendering(VkCommandBuffer cb);
    void onEvent();
    void toggleDisplay();

private:
    static void checkVkResult(VkResult err);

private:
    bool                        m_isShowing      = true;

    Unique<VkGfxDescriptorPool> m_descriptorPool = nullptr;
};
}; // namespace dusk