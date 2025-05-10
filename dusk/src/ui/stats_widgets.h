#pragma once

#include "dusk.h"
#include "engine.h"

#include <imgui.h>

namespace dusk
{
inline void drawStatsWidget()
{
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoDocking;
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin("Statistics", nullptr, windowFlags);

    ImGui::Text("FPS: %g", 1.0f / Engine::get().getFrameDelta().count());
    ImGui::Text("Mouse position: (%g, %g)", io.MousePos.x, io.MousePos.y);

    ImGui::SeparatorText("Controls");
    ImGui::Text("F4 to enable/disable UI");
    ImGui::Text("RMB to look around");
    ImGui::Text("W/S/A/D/Q/E with RMB to move camera");

    ImGui::End();
}
} // namespace dusk