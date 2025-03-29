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
                m_moveDirection = {};
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

                // 1.5 radians (~85 degrees) rotation for half screen
                float verticalAngle = deltaY * 2 * 1.5 / m_height;

                // 3.14 radians rotation for half screen
                float horizontalAngle = deltaX * 2 * 3.14 / m_width;

                // TODO: ineffecient code
                // TODO: explore rotational axis as camera's up and right vectors
                glm::quat newVerticalRotaion   = glm::angleAxis(verticalAngle, glm::vec3(1.0f, 0.f, 0.f));
                glm::quat newHorizontalRotaion = glm::angleAxis(horizontalAngle, glm::vec3(0.0f, 1.f, 0.f));

                if (m_isLeftShiftPressed)      // no vertical rotation
                    m_cameraTransform.rotation = newHorizontalRotaion * m_cameraTransform.rotation;
                else if (m_isLeftAltPressed) // no horizontal rotation
                    m_cameraTransform.rotation = newVerticalRotaion * m_cameraTransform.rotation;
                else                         // full rotation
                    m_cameraTransform.rotation = newHorizontalRotaion * newVerticalRotaion * m_cameraTransform.rotation;

                m_mouseX = ev.getX();
                m_mouseY = ev.getY();
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
                    m_moveDirection += m_cameraComponent.forwardDirection;
                }
                else if (ev.getKeyCode() == Key::S)
                {
                    m_moveDirection -= m_cameraComponent.forwardDirection;
                }
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
                    m_moveDirection -= m_cameraComponent.forwardDirection;
                }
                else if (ev.getKeyCode() == Key::S)
                {
                    m_moveDirection += m_cameraComponent.forwardDirection;
                }
            }

            if (ev.getKeyCode() == Key::LeftShift) m_isLeftShiftPressed = false;

            if (ev.getKeyCode() == Key::LeftAlt) m_isLeftAltPressed = false;

            return false;
        });
}

void CameraController::onUpdate(TimeStep dt)
{
    m_cameraTransform.translation += m_cameraMoveSpeed * dt.count() * m_moveDirection;

    m_cameraComponent.setView(m_cameraTransform.translation, m_cameraTransform.rotation);
}

} // namespace dusk