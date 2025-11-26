#include "camera_controller.h"

#include "core/mouse_codes.h"
#include "core/key_codes.h"
#include "events/mouse_event.h"
#include "events/key_event.h"
#include "events/app_event.h"

namespace dusk
{
CameraController::CameraController(
    GameObject& camera,
    uint32_t    width,
    uint32_t    height,
    glm::vec3   up) :
    m_cameraTransform(camera.getComponent<TransformComponent>()), m_cameraComponent(camera.getComponent<CameraComponent>()),
    m_width(width), m_height(height), m_upDir(glm::normalize(up))
{
    m_rightDir = glm::normalize(glm::cross(m_upDir, m_forwardDir));
    m_upDir    = glm::normalize(glm::cross(m_forwardDir, m_rightDir));
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
                // Also we will be using -world up and -camera right because
                // in projection y will be flipped,this will make sure our fps
                // movements are aligned properly
                glm::quat quatPitch        = glm::angleAxis(pitch, -m_rightDir);
                glm::quat quatYaw          = glm::angleAxis(yaw, glm::vec3(0.f, -1.f, 0.f));

                m_cameraTransform.rotation = quatYaw * quatPitch * m_cameraTransform.rotation;

                glm::mat3 rotationMat      = glm::mat3_cast(m_cameraTransform.rotation);
                m_rightDir                 = rotationMat[0];
                m_upDir                    = rotationMat[1];
                m_forwardDir               = rotationMat[2];

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

        if (m_isAPressed) moveDirection -= m_rightDir;
        if (m_isDPressed) moveDirection += m_rightDir;
        if (m_isWPressed) moveDirection -= m_forwardDir;
        if (m_isSPressed) moveDirection += m_forwardDir;
        if (m_isEPressed) moveDirection += m_upDir;
        if (m_isQPressed) moveDirection -= m_upDir;

        m_cameraTransform.translation += m_cameraMoveSpeed * dt.count() * moveDirection;
    }
    m_cameraComponent.setView(m_cameraTransform.translation, m_cameraTransform.rotation);
}

void CameraController::setViewDirection(glm::vec3 position, glm::vec3 direction)
{
    m_cameraTransform.translation = position;

    m_forwardDir                  = glm::normalize(direction);
    m_rightDir                    = glm::normalize(glm::cross(m_upDir, m_forwardDir));
    m_upDir                       = glm::cross(m_forwardDir, m_rightDir);

    glm::mat3 rotationMat         = { 1.0f };
    rotationMat[0]                = m_rightDir;
    rotationMat[1]                = m_upDir;
    rotationMat[2]                = m_forwardDir;

    m_cameraTransform.rotation    = glm::quat_cast(rotationMat);

    m_cameraComponent.setView(
        m_cameraTransform.translation,
        m_cameraTransform.rotation);
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

void CameraController::setPosition(glm::vec3 position)
{
    m_cameraTransform.translation = position;
    m_cameraComponent.setView(m_cameraTransform.translation, m_cameraTransform.rotation);
}
} // namespace dusk