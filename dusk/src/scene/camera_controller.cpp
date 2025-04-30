#include "camera_controller.h"
#include "core/mouse_codes.h"
#include "core/key_codes.h"
#include "events/mouse_event.h"
#include "events/key_event.h"
#include "events/app_event.h"

namespace dusk
{
CameraController::CameraController(GameObject& camera, float width, float height) :
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
                m_isRMBpressed    = true;

                m_mouseX          = ev.getClickedPosX();
                m_mouseY          = ev.getClickedPosY();
                m_angleHorizontal = 0.f;
                m_angleVertical   = 0.f;
            }

            return false;
        });

    dispatcher.dispatch<MouseButtonReleasedEvent>(
        [this](MouseButtonReleasedEvent& ev)
        {
            if (ev.getMouseButton() == Mouse::ButtonRight)
            {
                m_isRMBpressed  = false;
                m_changed       = false;
                m_moveDirection = {};
            }

            return false;
        });

    dispatcher.dispatch<MouseMovedEvent>(
        [this](MouseMovedEvent& ev)
        {
            if (m_isRMBpressed)
            {
                float     deltaX                = ev.getX() - m_mouseX;
                float     deltaY                = ev.getY() - m_mouseY;

                float     verticalAngle         = deltaY * 1.f * 1.5f / m_height;

                float     horizontalAngle       = deltaX * 0.75f * 3.14f / m_width;

                glm::quat newVerticalRotation   = glm::angleAxis(-verticalAngle, m_cameraComponent.rightDirection);
                
                // fix for avoiding unintended roll is to use world up for
                // yaw rotations
                // https://gamedev.stackexchange.com/questions/103242/why-is-the-camera-tilting-around-the-z-axis-when-i-only-specified-x-and-y/103243#103243
                glm::quat newHorizontalRotation = glm::angleAxis(-horizontalAngle, glm::vec3 { 0.f, 1.f, 0.f });

                if (m_isLeftShiftPressed)    // no vertical rotation
                    m_cameraTransform.rotation = newHorizontalRotation * m_cameraTransform.rotation;
                else if (m_isLeftAltPressed) // no horizontal rotation
                    m_cameraTransform.rotation = newVerticalRotation * m_cameraTransform.rotation;
                else                         // full rotation
                    m_cameraTransform.rotation = newHorizontalRotation * newVerticalRotation * m_cameraTransform.rotation;

                m_mouseX  = ev.getX();
                m_mouseY  = ev.getY();

                m_changed = true;
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
                    m_moveDirection -= m_cameraComponent.rightDirection;
                }
                else if (ev.getKeyCode() == Key::D)
                {
                    m_moveDirection += m_cameraComponent.rightDirection;
                }
                else if (ev.getKeyCode() == Key::Space || ev.getKeyCode() == Key::E)
                {
                    m_moveDirection -= m_cameraComponent.upDirection;
                }
                else if (ev.getKeyCode() == Key::LeftControl || ev.getKeyCode() == Key::Q)
                {
                    m_moveDirection += m_cameraComponent.upDirection;
                }
                else if (ev.getKeyCode() == Key::W)
                {
                    m_moveDirection += m_cameraComponent.forwardDirection;
                }
                else if (ev.getKeyCode() == Key::S)
                {
                    m_moveDirection -= m_cameraComponent.forwardDirection;
                }

                m_changed = true;
            }

            if (ev.getKeyCode() == Key::LeftShift) m_isLeftShiftPressed = true;

            if (ev.getKeyCode() == Key::LeftAlt) m_isLeftAltPressed = true;

            return false;
        });

    dispatcher.dispatch<KeyReleasedEvent>(
        [this](KeyReleasedEvent& ev)
        {
            if (m_isRMBpressed && m_moveDirection != glm::vec3 { 0.f, 0.f, 0.f })
            {
                if (ev.getKeyCode() == Key::A)
                {
                    m_moveDirection += m_cameraComponent.rightDirection;
                }
                else if (ev.getKeyCode() == Key::D)
                {
                    m_moveDirection -= m_cameraComponent.rightDirection;
                }
                else if (ev.getKeyCode() == Key::Space || ev.getKeyCode() == Key::E)
                {
                    m_moveDirection += m_cameraComponent.upDirection;
                }
                else if (ev.getKeyCode() == Key::LeftControl || ev.getKeyCode() == Key::Q)
                {
                    m_moveDirection -= m_cameraComponent.upDirection;
                }
                else if (ev.getKeyCode() == Key::W)
                {
                    m_moveDirection -= m_cameraComponent.forwardDirection;
                }
                else if (ev.getKeyCode() == Key::S)
                {
                    m_moveDirection += m_cameraComponent.forwardDirection;
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
        m_cameraTransform.translation += m_cameraMoveSpeed * dt.count() * m_moveDirection;
        m_cameraComponent.setView(m_cameraTransform.translation, m_cameraTransform.rotation);
    }
}

} // namespace dusk