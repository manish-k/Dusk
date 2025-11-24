#include "camera_controller.h"
#include "core/mouse_codes.h"
#include "core/key_codes.h"
#include "events/mouse_event.h"
#include "events/key_event.h"
#include "events/app_event.h"

namespace dusk
{
CameraController::CameraController(GameObject& camera, uint32_t width, uint32_t height) :
    m_cameraTransform(camera.getComponent<TransformComponent>()), m_cameraComponent(camera.getComponent<CameraComponent>()),
    m_width(width), m_height(height)
{
}

void CameraController::onEvent(Event& ev)
{
    EventDispatcher dispatcher(ev);

    dispatcher.dispatch<WindowResizeEvent>(
        [this](WindowResizeEvent& ev)
        {
            m_width  = ev.getWidth();
            m_height = ev.getHeight();
            return false;
        });

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](MouseButtonPressedEvent& ev)
        {
            if (ev.getMouseButton() == Mouse::ButtonRight)
            {
                m_isRMBpressed = true;

                m_mouseX       = ev.getClickedPosX();
                m_mouseY       = ev.getClickedPosY();

                m_isAPressed   = false;
                m_isDPressed   = false;
                m_isWPressed   = false;
                m_isSPressed   = false;
                m_isEPressed   = false;
                m_isQPressed   = false;
            }

            return false;
        });

    dispatcher.dispatch<MouseButtonReleasedEvent>(
        [this](MouseButtonReleasedEvent& ev)
        {
            if (ev.getMouseButton() == Mouse::ButtonRight)
            {
                m_isRMBpressed = false;
                m_changed      = false;

                m_isAPressed   = false;
                m_isDPressed   = false;
                m_isWPressed   = false;
                m_isSPressed   = false;
                m_isEPressed   = false;
                m_isQPressed   = false;
            }

            return false;
        });

    dispatcher.dispatch<MouseMovedEvent>(
        [this](MouseMovedEvent& ev)
        {
            if (m_isRMBpressed)
            {
                float deltaX = ev.getX() - m_mouseX;
                float deltaY = ev.getY() - m_mouseY;

                float yaw    = 0.f;
                float pitch  = 0.f;

                if (m_isLeftShiftPressed)    // no vertical rotation
                    yaw += deltaX * m_mouseSensitivity;
                else if (m_isLeftAltPressed) // no horizontal rotation
                    pitch += deltaY * m_mouseSensitivity;
                else                         // full rotation
                {
                    yaw += deltaX * m_mouseSensitivity;
                    pitch += deltaY * m_mouseSensitivity;
                }

                yaw = glm::mod(yaw, glm::two_pi<float>());

                // fix for avoiding unintended roll is to use world up for yaw rotations
                // https://gamedev.stackexchange.com/questions/103242/why-is-the-camera-tilting-around-the-z-axis-when-i-only-specified-x-and-y/103243#103243
                // Also we will be using -world up and -camera right because will be flipping y in projection,
                // this will make sure our fps movement are aligned properly
                glm::quat quatPitch        = glm::angleAxis(pitch, -m_cameraComponent.rightDirection);
                glm::quat quatYaw          = glm::angleAxis(yaw, glm::vec3(0.f, -1.f, 0.f));

                m_cameraTransform.rotation = quatYaw * quatPitch * m_cameraTransform.rotation;

                m_mouseX                   = ev.getX();
                m_mouseY                   = ev.getY();

                m_changed                  = true;
            }

            return false;
        });

    dispatcher.dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent& ev)
        {
            if (m_isRMBpressed)
            {
                if (ev.getKeyCode() == Key::A)
                {
                    m_isAPressed = true;
                }
                else if (ev.getKeyCode() == Key::D)
                {
                    m_isDPressed = true;
                }
                else if (ev.getKeyCode() == Key::Space || ev.getKeyCode() == Key::E)
                {
                    m_isEPressed = true;
                }
                else if (ev.getKeyCode() == Key::LeftControl || ev.getKeyCode() == Key::Q)
                {
                    m_isQPressed = true;
                }
                else if (ev.getKeyCode() == Key::W)
                {
                    m_isWPressed = true;
                }
                else if (ev.getKeyCode() == Key::S)
                {
                    m_isSPressed = true;
                }

                m_changed = true;
            }

            if (ev.getKeyCode() == Key::R)
            {
                resetCamera();
            }

            if (ev.getKeyCode() == Key::LeftShift) m_isLeftShiftPressed = true;

            if (ev.getKeyCode() == Key::LeftAlt) m_isLeftAltPressed = true;

            return false;
        });

    dispatcher.dispatch<KeyReleasedEvent>(
        [this](KeyReleasedEvent& ev)
        {
            if (m_isRMBpressed)
            {
                if (ev.getKeyCode() == Key::A)
                {
                    m_isAPressed = false;
                }
                else if (ev.getKeyCode() == Key::D)
                {
                    m_isDPressed = false;
                }
                else if (ev.getKeyCode() == Key::Space || ev.getKeyCode() == Key::E)
                {
                    m_isEPressed = false;
                }
                else if (ev.getKeyCode() == Key::LeftControl || ev.getKeyCode() == Key::Q)
                {
                    m_isQPressed = false;
                }
                else if (ev.getKeyCode() == Key::W)
                {
                    m_isWPressed = false;
                }
                else if (ev.getKeyCode() == Key::S)
                {
                    m_isSPressed = false;
                }

                m_changed = true;
            }

            if (ev.getKeyCode() == Key::LeftShift) m_isLeftShiftPressed = false;

            if (ev.getKeyCode() == Key::LeftAlt) m_isLeftAltPressed = false;

            return false;
        });
}

void CameraController::onUpdate(TimeStep dt)
{
    if (m_changed)
    {
        glm::vec3 moveDirection = {};

        if (m_isAPressed) moveDirection -= m_cameraComponent.rightDirection;
        if (m_isDPressed) moveDirection += m_cameraComponent.rightDirection;
        if (m_isWPressed) moveDirection -= m_cameraComponent.forwardDirection;
        if (m_isSPressed) moveDirection += m_cameraComponent.forwardDirection;
        if (m_isEPressed) moveDirection += m_cameraComponent.upDirection;
        if (m_isQPressed) moveDirection -= m_cameraComponent.upDirection;

        m_cameraTransform.translation += m_cameraMoveSpeed * dt.count() * moveDirection;
        m_cameraComponent.setView(m_cameraTransform.translation, m_cameraTransform.rotation);

        //DUSK_DEBUG("camera fwd dir {},{},{}", m_cameraComponent.forwardDirection.x, m_cameraComponent.forwardDirection.y, m_cameraComponent.forwardDirection.z);
    }
}

void CameraController::setViewDirection(glm::vec3 position, glm::vec3 direction)
{
    m_cameraTransform.translation = position;

    // Calculate yaw and pitch from direction
    direction                  = glm::normalize(direction);
    float     yaw              = atan2(direction.z, direction.x);
    float     pitch            = asin(direction.y);

    glm::quat quatPitch        = glm::angleAxis(pitch, -m_cameraComponent.rightDirection);
    glm::quat quatYaw          = glm::angleAxis(yaw, glm::vec3(0.f, -1.f, 0.f));

    m_cameraTransform.rotation = quatYaw * quatPitch * m_cameraTransform.rotation;

    m_cameraComponent.setView(m_cameraTransform.translation, m_cameraTransform.rotation);
    m_startTransform = m_cameraTransform;
}

void CameraController::setViewDirection(glm::vec3 direction)
{
    setViewDirection(m_cameraTransform.translation, direction);
}

void CameraController::setViewTarget(glm::vec3 position, glm::vec3 target)
{
    if (target == position)
    {
        target += glm::vec3(0.001f);
    }

    setViewDirection(position, target - position);
}

void CameraController::setViewTarget(glm::vec3 target)
{
    setViewTarget(m_cameraTransform.translation, target);
}

void CameraController::resetCamera()
{
    m_cameraTransform = m_startTransform; // copy back
    m_cameraComponent.setView(m_cameraTransform.translation, m_cameraTransform.rotation);
}

} // namespace dusk