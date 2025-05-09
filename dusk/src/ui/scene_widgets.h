#pragma once

#include "dusk.h"
#include "scene/scene.h"

#include <imgui.h>
#include <glm/gtx/euler_angles.hpp>

namespace dusk
{

inline void drawGameObjectSubTree(Scene& scene, EntityId objectId)
{
    ImGuiTreeNodeFlags flags      = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;

    auto&              gObject    = scene.getGameObject(objectId);
    auto&              children   = gObject.getChildren();
    SceneUIState&      sceneState = UI::state().sceneState;

    if (children.size() > 0)
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    EntityId selectedGameObjectId = sceneState.selectedGameObjectId;
    if (selectedGameObjectId == gObject.getId())
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool nodeOpen = ImGui::TreeNodeEx(gObject.getCName(), flags);

    if (ImGui::IsItemClicked())
    {
        sceneState.selectedGameObjectId = gObject.getId();
    }

    if (nodeOpen)
    {
        // render all children
        for (const auto& childId : children)
        {
            drawGameObjectSubTree(scene, childId);
        }
        ImGui::TreePop();
    }
}

inline void drawSceneGraphWidget(Scene& scene)
{
    SceneUIState& sceneState = UI::state().sceneState;

    ImGui::Begin("Scene");

    ImGui::SeparatorText("Hierarchy");

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("Hierarchy", ImVec2(ImGui::GetContentRegionAvail().x * 1.f, 260), ImGuiChildFlags_None, window_flags);
    drawGameObjectSubTree(scene, scene.getRootId());
    ImGui::EndChild();

    if (sceneState.selectedGameObjectId != NULL_ENTITY)
    {
        auto& selectedGameObject = scene.getGameObject(sceneState.selectedGameObjectId);

        ImGui::SeparatorText("Transform");

        auto& transform = selectedGameObject.getComponent<TransformComponent>();

        ImGui::DragFloat3("Position", (float*)&transform.translation);
        ImGui::DragFloat3("Scale", (float*)&transform.scale);

        glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(transform.rotation));
        if (ImGui::DragFloat3("Rotation", (float*)&eulerRotation, 0.5f))
        {
            transform.setRotation(glm::quat(eulerRotation));
        }

        if (selectedGameObject.hasComponent<CameraComponent>())
        {
            ImGui::SeparatorText("Camera");

            CameraComponent& camera      = selectedGameObject.getComponent<CameraComponent>();

            float            nearPlane   = camera.nearPlane;
            float            farPlane    = camera.farPlane;
            float            aspectRatio = camera.aspectRatio;
            float            fovy        = glm::degrees(camera.fovY);

            bool             changed     = false;
            if (ImGui::DragFloat("Near Plane", &nearPlane, 0.05f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_None)) changed = true;
            if (ImGui::DragFloat("Far Plane", &farPlane, 0.05f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_None)) changed = true;
            if (ImGui::DragFloat("FOV (Degree)", &fovy, 0.05f, 0.0f, 80, "%.3f", ImGuiSliderFlags_None)) changed = true;

            if (changed)
            {
                camera.setPerspectiveProjection(glm::radians(fovy), aspectRatio, nearPlane, farPlane);
            }
        }
    }

    ImGui::End();
}

} // namespace dusk