#pragma once

#include "dusk.h"
#include "scene/scene.h"

#include <imgui.h>

namespace dusk
{

inline void drawGameObjectSubTree(Scene& scene, EntityId objectId)
{
    ImGuiTreeNodeFlags flags    = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;

    auto&              gObject  = scene.getGameObject(objectId);
    auto&              children = gObject.getChildren();

    if (children.size() > 0)
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (ImGui::TreeNodeEx(gObject.getCName(), flags))
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
    ImGui::Begin("Scene");

    drawGameObjectSubTree(scene, scene.getRootId());

    ImGui::End();
}

} // namespace dusk