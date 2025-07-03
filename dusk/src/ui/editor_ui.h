#pragma once

#include "dusk.h"
#include "ui_states.h"
#include "scene/scene.h"
#include "platform/window.h"
#include "backend/vulkan/vk_descriptors.h"

#include <volk.h>

namespace dusk
{
class EditorUI
{
public:
    EditorUI()  = default;
    ~EditorUI() = default;

    bool            init(Window& window);
    void            shutdown();
    void            beginRendering();
    void            renderCommonWidgets();
    void            endRendering(VkCommandBuffer cb);
    void            renderSceneWidgets(Scene& scene);
    void            onEvent(Event& ev);

    static UIState& state() { return s_uiState; }

private:
    void        setupImGuiTheme();
    static void checkVkResult(VkResult err);

private:
    bool                        m_isShowing      = true;

    Unique<VkGfxDescriptorPool> m_descriptorPool = nullptr;

private:
    static UIState s_uiState;
};
}; // namespace dusk