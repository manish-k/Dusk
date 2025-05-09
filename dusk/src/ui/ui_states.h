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
};

struct UIState
{
    RendererUIState rendererState;
    SceneUIState    sceneState;
};

} // namespace dusk