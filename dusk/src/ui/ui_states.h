#pragma once

#include "scene/scene.h"

namespace dusk
{

struct RendererUIState
{
    bool useSkybox = true;
};

struct SceneUIState
{
    EntityId selectedGameObjectId = NULL_ENTITY;
    int      selectedMeshId       = -1;
};

struct UIState
{
    RendererUIState rendererState;
    SceneUIState    sceneState;
};

} // namespace dusk