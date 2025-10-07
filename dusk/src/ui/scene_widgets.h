#pragma once

#include "dusk.h"
#include "scene/scene.h"

#include "components/camera.h"
#include "components/mesh.h"
#include "components/lights.h"
#include "components/transform.h"

#include "renderer/material.h"

#include <imgui.h>
#include <glm/gtx/euler_angles.hpp>

namespace dusk
{

inline void drawGameObjectSubTree(Scene& scene, EntityId objectId)
{
    ImGuiTreeNodeFlags flags      = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;

    auto&              gObject    = scene.getGameObject(objectId);
    auto&              children   = gObject.getChildren();
    SceneUIState&      sceneState = EditorUI::state().sceneState;

    if (children.size() > 0)
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    EntityId selectedGameObjectId = sceneState.selectedGameObjectId;
    if (selectedGameObjectId == gObject.getId())
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool nodeOpen = ImGui::TreeNodeEx((void*)gObject.getId(), flags, gObject.getCName());

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
    SceneUIState& sceneState = EditorUI::state().sceneState;

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

        ImGui::DragFloat3("Position", (float*)&transform.translation, 0.005f, -FLT_MAX, FLT_MAX);
        ImGui::DragFloat3("Scale", (float*)&transform.scale);

        glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(transform.rotation));
        if (ImGui::DragFloat3("Rotation", (float*)&eulerRotation, 0.5f))
        {
            transform.setRotation(glm::quat(eulerRotation));
        }

        if (selectedGameObject.hasComponent<MeshComponent>())
        {
            MeshComponent& mesh = selectedGameObject.getComponent<MeshComponent>();

            ImGui::SeparatorText("Material");

            for (uint32_t index = 0u; index < mesh.materials.size(); ++index)
            {
                Material& mat = scene.getMaterial(mesh.materials[index]);
                ImGui::ColorEdit4("Albedo Color", (float*)&mat.albedoColor);
                ImGui::Text("Albedo Texture id: %d", mat.albedoTexId);
                ImGui::Text("Normal Texture id: %d", mat.normalTexId);
                ImGui::Text("Emissive Texture id: %d", mat.emissiveTexId);
                ImGui::Text("Metal-Rough Texture id: %d", mat.metallicRoughnessTexId);

                ImGui::SliderFloat("Roughness", &mat.rough, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);
                ImGui::SliderFloat("Metallic", &mat.metal, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);
            }
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

        if (selectedGameObject.hasComponent<AmbientLightComponent>())
        {
            ImGui::SeparatorText("Ambient Light");

            AmbientLightComponent& light = selectedGameObject.getComponent<AmbientLightComponent>();

            ImGui::ColorEdit3("Color", (float*)&light.color);
            ImGui::DragFloat("Intensity", (float*)&light.color + 3, 0.005f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_None);
        }

        if (selectedGameObject.hasComponent<DirectionalLightComponent>())
        {
            ImGui::SeparatorText("Directional Light");

            DirectionalLightComponent& light = selectedGameObject.getComponent<DirectionalLightComponent>();

            ImGui::DragFloat3("Direction", (float*)&light.direction);
            ImGui::ColorEdit3("Color", (float*)&light.color);
            ImGui::DragFloat("Intensity", (float*)&light.color + 3, 0.005f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_None);
        }

        if (selectedGameObject.hasComponent<PointLightComponent>())
        {
            ImGui::SeparatorText("Point Light");

            PointLightComponent& light = selectedGameObject.getComponent<PointLightComponent>();

            ImGui::ColorEdit3("Color", (float*)&light.color);
            ImGui::DragFloat("Intensity", (float*)&light.color + 3, 0.005f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_None);

            ImGui::Text("Attenuation Factors");
            ImGui::DragFloat("Constant", (float*)&light.constantAttenuationFactor, 0.05f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);
            ImGui::DragFloat("Linear", (float*)&light.linearAttenuationFactor, 0.05f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);
            ImGui::DragFloat("Quadratic", (float*)&light.quadraticAttenuationFactor, 0.05f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);
        }

        if (selectedGameObject.hasComponent<SpotLightComponent>())
        {
            ImGui::SeparatorText("Spot Light");

            SpotLightComponent& light = selectedGameObject.getComponent<SpotLightComponent>();

            float               innerAngle = glm::degrees(glm::acos(light.innerCutOff));
            float               outerAngle = glm::degrees(glm::acos(light.outerCutOff));

            ImGui::DragFloat3("Direction", (float*)&light.direction);
            ImGui::ColorEdit3("Color", (float*)&light.color);
            ImGui::DragFloat("Intensity", (float*)&light.color + 3, 0.005f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_None);

            ImGui::Text("Attenuation Factors");
            ImGui::DragFloat("Constant", (float*)&light.constantAttenuationFactor, 0.05f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);
            ImGui::DragFloat("Linear", (float*)&light.linearAttenuationFactor, 0.05f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);
            ImGui::DragFloat("Quadratic", (float*)&light.quadraticAttenuationFactor, 0.05f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);

            ImGui::Text("Cone CutOff Angles");
            if (ImGui::DragFloat("Inner Angle", (float*)&innerAngle, 0.05f, 0.0f, 90.0f, "%.3f", ImGuiSliderFlags_None))
            {
                light.innerCutOff = glm::cos(glm::radians(innerAngle));
            }

            if (ImGui::DragFloat("Outer Angle", (float*)&outerAngle, 0.05f, innerAngle, 90.0f, "%.3f", ImGuiSliderFlags_None))
            {
                light.outerCutOff = glm::cos(glm::radians(outerAngle));
            }
        }
    }

    ImGui::End();
}

} // namespace dusk