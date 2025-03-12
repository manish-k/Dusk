#pragma once

#include "dusk.h"

#include <glm/glm.hpp>
#include <limits>

namespace dusk
{
struct CameraComponent
{
    glm::mat4 projectionMatrix  = glm::mat4 { 1.0f };
    glm::mat4 viewMatrix        = glm::mat4 { 1.0f };
    glm::mat4 inverseViewMatrix = glm::mat4 { 1.0f };

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
        projectionMatrix       = glm::mat4 { 1.0f };
        projectionMatrix[0][0] = 2.f / (right - left);
        projectionMatrix[1][1] = 2.f / (bottom - top);
        projectionMatrix[2][2] = 1.f / (f - n);
        projectionMatrix[3][0] = -(right + left) / (right - left);
        projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
        projectionMatrix[3][2] = -n / (f - n);
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

        const float tanHalfFovy = tan(fovy / 2.f);
        projectionMatrix        = glm::mat4 { 0.0f };
        projectionMatrix[0][0]  = 1.f / (aspect * tanHalfFovy);
        projectionMatrix[1][1]  = 1.f / (tanHalfFovy);
        projectionMatrix[2][2]  = f / (f - n);
        projectionMatrix[2][3]  = 1.f;
        projectionMatrix[3][2]  = -(f * n) / (f - n);
    };

    /**
     * @brief Set view direction
     * @param position of the camera
     * @param lookAtDirection direction to look at
     * @param up vector of camera
     */
    void setViewDirection(glm::vec3 position, glm::vec3 lookAtDirection, glm::vec3 up)
    {
        const glm::vec3 w { glm::normalize(lookAtDirection) };
        const glm::vec3 u { glm::normalize(glm::cross(w, up)) };
        const glm::vec3 v { glm::cross(w, u) };

        viewMatrix = glm::mat4 { 1.f };

        /// TODO: reasoning not clear
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;

        // moving camera back to oriign
        viewMatrix[3][0] = -glm::dot(u, position);
        viewMatrix[3][1] = -glm::dot(v, position);
        viewMatrix[3][2] = -glm::dot(w, position);

        // inverse calculations
        inverseViewMatrix       = glm::mat4 { 1.f };
        inverseViewMatrix[0][0] = u.x;
        inverseViewMatrix[0][1] = u.y;
        inverseViewMatrix[0][2] = u.z;
        inverseViewMatrix[1][0] = v.x;
        inverseViewMatrix[1][1] = v.y;
        inverseViewMatrix[1][2] = v.z;
        inverseViewMatrix[2][0] = w.x;
        inverseViewMatrix[2][1] = w.y;
        inverseViewMatrix[2][2] = w.z;
        inverseViewMatrix[3][0] = position.x;
        inverseViewMatrix[3][1] = position.y;
        inverseViewMatrix[3][2] = position.z;
    };

    /**
     * @brief Set view target of camera
     * @param position of the camera
     * @param target position
     * @param up vector of camera
     */
    void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up)
    {
        setViewDirection(position, target - position, up);
    };
};
} // namespace dusk