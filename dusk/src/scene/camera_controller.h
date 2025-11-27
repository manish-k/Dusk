#pragma once

/**
 * @brief Camera system with consistent RHS coordinate convention
 *
 * COORDINATE SYSTEM:
 * ==================
 * World Space:      Right-Handed, Y-up, -Z forward (standard 3D convention)
 * Camera/View:      Right-Handed, Y-up, -Z forward (MATCHES world space)
 * Clip/NDC:         Vulkan convention (Y-down, Z ∈ [0,1])
 *
 * TRANSFORMATION FLOW:
 * ====================
 *   World (RHS)  →  [View]  →  Camera (RHS)  →  [Projection*]  →  Clip (Vulkan NDC)
 *       ↓                          ↓                                      ↓
 *    -Z fwd                     -Z fwd                           Y-down & +Z forward
 *
 * * Projection matrix includes Y-flip: projection[1][1] *= -1.0f
 *
 * CAMERA AXES (match world):
 * ==========================
 * - Right:   +X (world and camera)
 * - Up:      +Y (world and camera)
 * - Forward: -Z (camera looks down -Z)
 */

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

    /**
     * @brief Sets the camera's view using a world-space position and a forward direction vector.
     * @param position The world-space position of the camera.
     * @param direction The forward direction vector that the view should point toward.
     */
    void setViewDirection(glm::vec3 position, glm::vec3 direction);
    
    /**
     * @brief Sets the current view direction.
     * @param direction The new view direction as a 3D vector (glm::vec3).
     */
    void setViewDirection(glm::vec3 direction);
    
    /**
     * @brief Sets the view by specifying the camera position and the point to look at.
     * @param position The camera position in world coordinates.
     * @param target The point in world coordinates that the camera should look at.
     */
    void setViewTarget(glm::vec3 position, glm::vec3 target);
    
    /**
     * @brief Sets the view to look at a target point from the current camera position.
     * @param target The point in world coordinates that the camera should look at.
     */
    void setViewTarget(glm::vec3 target);
    
    /**
     * @brief Sets the camera's position in world space.
     * @param position The new position of the camera as a 3D vector (glm::vec3).
     */
    void setPosition(glm::vec3 position);
    
    /**
     * @brief Resets the camera to its default state provided by set view functions.
     */
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