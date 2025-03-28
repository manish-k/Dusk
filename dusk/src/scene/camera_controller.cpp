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

                m_angleHorizontal += deltaX / 20.0f;
                m_angleVertical += deltaY / 50.0f;

                m_mouseX = ev.getX();
                m_mouseY = ev.getY();

                DUSK_INFO("horizontal {} delta {}", m_angleHorizontal, deltaX);
                DUSK_INFO("vertical {} delta {}", m_angleVertical, deltaY);
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

            return false;
        });
}

void CameraController::onUpdate(TimeStep dt)
{
    m_cameraTransform.translation += m_cameraMoveSpeed * dt.count() * m_moveDirection;

    m_cameraComponent.setView(m_cameraTransform.translation, m_cameraTransform.rotation);
}

} // namespace dusk