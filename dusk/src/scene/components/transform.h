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
    ;
    glm::vec3 scale = glm::vec3 { 1.0f };

    glm::mat4 Mat4()
    {
        return glm::mat4 {};
    }

    glm::mat4 NormalMat4()
    {
        return glm::mat4 {};
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
