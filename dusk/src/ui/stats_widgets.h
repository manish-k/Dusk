#pragma once

#include "dusk.h"
#include "engine.h"
#include "stats_recorder.h"

#include <imgui.h>

namespace dusk
{
inline void drawStatsWidget()
{
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoDocking;

    const auto* StatsRecorder  = StatsRecorder::get();
    const auto  stats          = StatsRecorder->getThirdLastFrameStats();
    const auto  aggregateStats = StatsRecorder->getAggregateStats();

    ImGui::Begin("Statistics", nullptr, windowFlags);

    float cpuTimeMs    = stats.cpuFrameTime.count() / 1000000.0f;
    float emaCpuTimeMs = aggregateStats.emaCpuTimeNs.count() / 1000000.0f;

    float fps          = 1000.0f / emaCpuTimeMs;
    ImGui::Text("FPS: %g", fps);

    ImGui::SeparatorText("CPU");

    ImGui::Text("CPU time: %g ms", emaCpuTimeMs);

    ImGui::SeparatorText("GPU");

    float gpuTimeMs    = stats.gpuFrameTime.count() / 1000000.0f;
    float emaGpuTimeMs = aggregateStats.emaGpuTimeNs.count() / 1000000.0f;
    ImGui::Text("GPU time: %g ms", emaGpuTimeMs);

    if (ImGui::BeginTable("Pass Stats", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Pass Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("GPU Time (ms)", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();
        for (const auto& passStats : stats.passStats)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", passStats.passName.c_str());
            ImGui::TableNextColumn();
            float passGpuTimeMs = passStats.gpuTimeNs.count() / 1000000.0f;
            ImGui::Text("%g ms", passGpuTimeMs);
        }
        ImGui::EndTable();
    }

    ImGui::SeparatorText("VRAM");
    uint64_t totalVramUsedMB       = stats.totalVramUsedBytes / (1024 * 1024);
    uint64_t totalVramBudgetGB    = stats.totalVramBudgetBytes / (1024 * 1024 * 1024);
    uint64_t totalVramGB           = stats.totalVram / (1024 * 1024 * 1024);
    uint64_t totalVramBufferUsedMB = stats.totalVramBufferUsedBytes / (1024 * 1024);
    uint64_t totalVramImageUsedMB  = stats.totalVramImageUsedBytes / (1024 * 1024);

    ImGui::Text("Total Used: %llu MB / %llu GB", totalVramUsedMB, totalVramBudgetGB);
    //ImGui::Text("Buffers:    %llu MB", totalVramBufferUsedMB);
    //ImGui::Text("Images:     %llu MB", totalVramImageUsedMB);

    ImGui::SeparatorText("Controls");
    ImGui::Text("F4 to enable/disable UI");
    ImGui::Text("F6 to dump render graph dot file");
    ImGui::Text("RMB to look around");
    ImGui::Text("W/S/A/D/Q/E with RMB to move camera");

    ImGui::End();
}
} // namespace dusk