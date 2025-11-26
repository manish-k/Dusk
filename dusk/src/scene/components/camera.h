#pragma once

#include "dusk.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

namespace dusk
{
struct CameraComponent
{
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
    void setOrthographicProjection(
        float left,
        float right,
        float top,
        float bottom,
        float n,
        float f)
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
     * @brief Set view as per camera's position and rotation
     * @param position position of the camera
     * @param total rotation to be applied on the camera
     */
    void setView(glm::vec3 position, glm::quat rotation)
    {
        glm::mat3 rotationMat = glm::mat3_cast(rotation);
        glm::vec3 right       = rotationMat[0];
        glm::vec3 up          = rotationMat[1];
        glm::vec3 forward     = rotationMat[2];

        updateViewMatrix(position, right, up, forward);
    }

    /**
     * @brief update view and inverseview matrices for given position and camera's bases vectors
     * @param position of the camera
     * @param Camera's right direction
     * @param Camera's up direction
     * @param Camera's forward direction
     */
    void updateViewMatrix(glm::vec3 position, glm::vec3 right, glm::vec3 up, glm::vec3 fwd)
    {

        // Rotation part
        viewMatrix    = glm::mat4(1.0f);
        viewMatrix[0] = glm::vec4(right.x, up.x, fwd.x, 0.0f);
        viewMatrix[1] = glm::vec4(right.y, up.y, fwd.y, 0.0f);
        viewMatrix[2] = glm::vec4(right.z, up.z, fwd.z, 0.0f);

        // Translation part
        viewMatrix[3] = glm::vec4(
            -glm::dot(right, position),
            -glm::dot(up, position),
            -glm::dot(fwd, position),
            1.0f);

        // Inverse: just transpose rotation and use original position
        inverseViewMatrix    = glm::mat4(1.0f);
        inverseViewMatrix[0] = glm::vec4(right.x, right.y, right.z, 0.0f);
        inverseViewMatrix[1] = glm::vec4(up.x, up.y, up.z, 0.0f);
        inverseViewMatrix[2] = glm::vec4(fwd.x, fwd.y, fwd.z, 0.0f);
        inverseViewMatrix[3] = glm::vec4(position, 1.0f);
    }
};
} // namespace dusk