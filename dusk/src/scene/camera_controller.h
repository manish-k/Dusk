#pragma once

#include "dusk.h"
#include "game_object.h"
#include "events/event.h"
#include "core/dtime.h"
#include "components/transform.h"
#include "components/camera.h"

#include <glm/glm.hpp>

namespace dusk
{
/**
 * @brief Camera Controller class is for handling keyboard and mouse events
 */
class CameraController
{
public:
    CameraController(GameObject& camera, float width, float height);
    ~CameraController() = default;

    void onEvent(Event& ev);
    void onUpdate(TimeStep dt);

private:
    TransformComponent& m_cameraTransform;
    CameraComponent&    m_cameraComponent;
    float               m_width              = 0.0f;
    float               m_height             = 0.0f;

    float               m_cameraMoveSpeed    = { 4.0f };
    float               m_cameraLookSpeed    = { 1.5f };

    bool                m_isRMBpressed       = false;
    bool                m_isLeftShiftPressed = false;
    bool                m_isLeftAltPressed   = false;

    glm::vec3           m_moveDirection {};
    float               m_mouseX          = 0.f;
    float               m_mouseY          = 0.f;
    float               m_angleHorizontal = 0.f;
    float               m_angleVertical   = 0.f;
};
} // namespace dusk