#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace dusk
{
struct TransformComponent
{
    glm::vec3 translation = glm::vec3 { 0.0f };
    glm::quat rotation    = glm::quat(1.0, 0.0, 0.0, 0.0);
    glm::vec3 scale       = glm::vec3 { 1.0f };

    glm::mat4 mat4()
    {
        return glm::mat4 {
            {
                scale.x * (1 - 2 * (rotation.y * rotation.y + rotation.z * rotation.z)),
                scale.x * (2 * (rotation.x * rotation.y + rotation.w * rotation.z)),
                scale.x * (2 * (rotation.x * rotation.z - rotation.w * rotation.y)),
                0.0f,
            },
            {
                scale.y * (2 * (rotation.x * rotation.y - rotation.w * rotation.z)),
                scale.y * (1 - 2 * (rotation.x * rotation.x + rotation.z * rotation.z)),
                scale.y * (2 * (rotation.y * rotation.z + rotation.w * rotation.x)),
                0.0f,
            },
            {
                scale.z * (2 * (rotation.x * rotation.z + rotation.w * rotation.y)),
                scale.z * (2 * (rotation.y * rotation.z - rotation.w * rotation.x)),
                scale.z * (1 - 2 * (rotation.y * rotation.y + rotation.x * rotation.x)),
                0.0f,
            },
            { translation.x, translation.y, translation.z, 1.0f }
        };
    }

    glm::mat4 normalMat4()
    {
        const glm::vec3 inverseScale = 1.0f / scale;

        return glm::mat4 {
            {
                inverseScale.x * (1 - 2 * (rotation.y * rotation.y + rotation.z * rotation.z)),
                inverseScale.x * (2 * (rotation.x * rotation.y + rotation.w * rotation.z)),
                inverseScale.x * (2 * (rotation.x * rotation.z - rotation.w * rotation.y)),
                0.0f,
            },
            {
                inverseScale.y * (2 * (rotation.x * rotation.y - rotation.w * rotation.z)),
                inverseScale.y * (1 - 2 * (rotation.x * rotation.x + rotation.z * rotation.z)),
                inverseScale.y * (2 * (rotation.y * rotation.z + rotation.w * rotation.x)),
                0.0f,
            },
            {
                inverseScale.z * (2 * (rotation.x * rotation.z + rotation.w * rotation.y)),
                inverseScale.z * (2 * (rotation.y * rotation.z - rotation.w * rotation.x)),
                inverseScale.z * (1 - 2 * (rotation.x * rotation.x + rotation.y * rotation.y)),
                0.0f,
            },
            { 0.f, 0.f, 0.f, 0.f }
        };
        //return glm::transpose(glm::inverse(glm::mat3(this->mat4())));
    }

    void setTranslation(const glm::vec3& newTranslation)
    {
        translation = newTranslation;
    }

    const glm::vec3& getTranslation() const
    {
        return translation;
    }

    void setRotation(const glm::quat& newRotation)
    {
        rotation = newRotation;
    }

    const glm::quat& getRotation() const
    {
        return rotation;
    }

    void setScale(const glm::vec3& newScale)
    {
        scale = newScale;
    }

    const glm::vec3& getScale() const
    {
        return scale;
    }
};
} // namespace dusk
