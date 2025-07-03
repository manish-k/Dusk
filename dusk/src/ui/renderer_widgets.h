#pragma once

#include "dusk.h"
#include "engine.h"
#include "ui_states.h"

#include <imgui.h>

namespace dusk
{
inline void drawRendererWidget()
{
    RendererUIState& rendererState = EditorUI::state().rendererState;

    ImGui::Begin("Renderer");

    ImGui::Checkbox("Skybox", &rendererState.useSkybox);

    ImGui::End();
}
} // namespace dusk