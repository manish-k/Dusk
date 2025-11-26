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
    CameraController(
        GameObject& camera, 
        uint32_t width, 
        uint32_t height,
        glm::vec3 up);
    ~CameraController() = default;

    void onEvent(Event& ev);
    void onUpdate(TimeStep dt);

    void setViewDirection(glm::vec3 position, glm::vec3 direction);
    void setViewDirection(glm::vec3 direction);
    void setViewTarget(glm::vec3 position, glm::vec3 target);
    void setViewTarget(glm::vec3 target);
    void setPosition(glm::vec3 position);
    void resetCamera();

    glm::vec3 getRightDirection() const { return m_rightDir; };
    glm::vec3 getUpDirection() const { return m_upDir; };
    glm::vec3 getForwardDirection() const { return m_forwardDir; };

private:
    TransformComponent& m_cameraTransform;
    CameraComponent&    m_cameraComponent;
    TransformComponent  m_startTransform   = {};

    float               m_width            = 0.0f;
    float               m_height           = 0.0f;

    glm::vec3           m_rightDir         = glm::vec3(1.f, 0.f, 0.f);
    glm::vec3           m_upDir            = glm::vec3(0.f, 1.f, 0.f);
    glm::vec3           m_forwardDir       = glm::vec3(0.f, 0.f, -1.f);

    bool                m_changed          = false; // TODO:: might not be required

    float               m_cameraMoveSpeed  = 20.0f;
    float               m_mouseSensitivity = 0.002f;

    // TODO:: create input state controller to track such states
    bool  m_isRMBpressed       = false;
    bool  m_isLeftShiftPressed = false;
    bool  m_isLeftAltPressed   = false;
    bool  m_isAPressed         = false;
    bool  m_isDPressed         = false;
    bool  m_isWPressed         = false;
    bool  m_isSPressed         = false;
    bool  m_isEPressed         = false;
    bool  m_isQPressed         = false;

    float m_mouseX             = 0.f;
    float m_mouseY             = 0.f;
};
} // namespace dusk