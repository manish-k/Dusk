#pragma once

#include "dusk.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

namespace dusk
{
constexpr glm::vec3 baseForwardDir = glm::vec3 { 0.f, 0.f, 1.f };
constexpr glm::vec3 baseRightDir   = glm::vec3 { 1.f, 0.f, 0.f };
constexpr glm::vec3 baseUpDir      = glm::vec3 { 0.f, 1.f, 0.f };

struct CameraComponent
{
    glm::vec3 forwardDirection        = baseForwardDir;
    glm::vec3 rightDirection          = baseRightDir;
    glm::vec3 upDirection             = baseUpDir;

    glm::mat4 projectionMatrix        = glm::mat4 { 1.0f };
    glm::mat4 viewMatrix              = glm::mat4 { 1.0f };
    glm::mat4 inverseViewMatrix       = glm::mat4 { 1.0f };
    glm::mat4 inverseProjectionMatrix = glm::mat4 { 1.0f };

    float     leftPlane               = 0.f;
    float     rightPlane              = 0.f;
    float     topPlane                = 0.f;
    float     bottomPlane             = 0.f;
    float     nearPlane               = 0.f;
    float     farPlane                = 0.f;
    float     aspectRatio             = 0.f;
    float     fovY                    = 0.f;

    bool      isPerspective           = false;

    /**
     * @brief Set camera projection as orthographic
     * @param left plane of the projection volume
     * @param right plane of the projection volume
     * @param top top plane of the projection volume
     * @param bottom plane of the projection volume
     * @param n near plane
     * @param f far plane
     */
    void setOrthographicProjection(float left, float right, float top, float bottom, float n, float f)
    {
        leftPlane        = left;
        rightPlane       = right;
        topPlane         = top;
        bottomPlane      = bottom;
        nearPlane        = n;
        farPlane         = f;
        isPerspective    = false;

        projectionMatrix = glm::mat4 { 1.0f };
        projectionMatrix = glm::orthoRH_ZO(left, right, bottom, top, n, f);
        projectionMatrix[1][1] *= -1; // Flip Y

        inverseProjectionMatrix = glm::inverse(projectionMatrix);
    };

    /**
     * @brief Set camera's projection as perspective
     * @param fovy frustum angle
     * @param aspect ratio
     * @param n near plane
     * @param f far plane
     */
    void setPerspectiveProjection(float fovy, float aspect, float n, float f)
    {
        DASSERT(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

        nearPlane        = n;
        farPlane         = f;
        aspectRatio      = aspect;
        fovY             = fovy;
        isPerspective    = true;

        projectionMatrix = glm::perspectiveRH_ZO(fovy, aspect, n, f);
        projectionMatrix[1][1] *= -1; // Flip Y

        inverseProjectionMatrix = glm::inverse(projectionMatrix);
    };

    void setAspectRatio(float aspect)
    {
        if (isPerspective)
        {
            setPerspectiveProjection(fovY, aspect, nearPlane, farPlane);
        }
    }

    /**
     * @brief Set view direction
     * @param position of the camera
     * @param lookAt direction to look at
     * @param up vector of camera
     */
    void setViewDirection(glm::vec3 position, glm::vec3 lookAt, glm::vec3 up)
    {
        forwardDirection = glm::normalize(lookAt);
        rightDirection   = glm::normalize(glm::cross(forwardDirection, glm::normalize(up)));
        upDirection      = glm::cross(forwardDirection, rightDirection);

        updateViewMatrix(position, rightDirection, upDirection, forwardDirection);
    };

    /**
     * @brief Set view target of camera
     * @param position of the camera
     * @param target position
     * @param up vector of camera
     */
    void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up)
    {
        if (target == position)
        {
            target += glm::vec3(0.001f);
        }
        setViewDirection(position, target - position, up);
    };

    /**
     * @brief Set view as per camera's position and rotation
     * @param position position of the camera
     * @param total rotation to be applied on the camera
     */
    void setView(glm::vec3 position, glm::quat rotation)
    {
        rightDirection   = glm::normalize(rotation * baseRightDir);
        upDirection      = glm::normalize(rotation * baseUpDir);
        forwardDirection = glm::normalize(rotation * baseForwardDir);

        updateViewMatrix(position, rightDirection, upDirection, forwardDirection);
    }

    /**
     * @brief update view and inverseview matrices for given position and camera's bases vectors
     * @param position of the camera
     * @param u basis in right direction perpendicular to v and w
     * @param v basis of camera toward up direction
     * @param w basis of camera toward lookAt direction
     */
    void updateViewMatrix(glm::vec3 position, glm::vec3 u, glm::vec3 v, glm::vec3 w)
    {
        // Build rotation part (camera basis as rows)
        viewMatrix    = glm::mat4(1.0f);
        viewMatrix[0] = glm::vec4(u.x, v.x, w.x, 0.0f);
        viewMatrix[1] = glm::vec4(u.y, v.y, w.y, 0.0f);
        viewMatrix[2] = glm::vec4(u.z, v.z, w.z, 0.0f);

        // Translation part (rotate the camera position, then negate)
        viewMatrix[3] = glm::vec4(
            -glm::dot(u, position),
            -glm::dot(v, position),
            -glm::dot(w, position),
            1.0f);

        // Inverse: just transpose rotation and use original position
        inverseViewMatrix    = glm::mat4(1.0f);
        inverseViewMatrix[0] = glm::vec4(u.x, u.y, u.z, 0.0f);
        inverseViewMatrix[1] = glm::vec4(v.x, v.y, v.z, 0.0f);
        inverseViewMatrix[2] = glm::vec4(w.x, w.y, w.z, 0.0f);
        inverseViewMatrix[3] = glm::vec4(position, 1.0f);
    }
};
} // namespace dusk