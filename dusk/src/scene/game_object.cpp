#include "game_object.h"

#include "transform_system.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace dusk
{
GameObject::GameObject()
{
    m_name            = "New GameObject";
    m_transformHandle = TransformSystem::create(getId(), NULL_ENTITY);
}

GameObject::~GameObject()
{
    // DUSK_DEBUG("Destroying GameObject {}", m_name);
}

void GameObject::addChild(GameObject& child)
{
    // TODO add assertion for existing child
    child.setParent(getId());
    m_children.push_back(child.getId());
}

void GameObject::removeChild(GameObject& child)
{
    // TODO add assertion for existing child
    child.setParent(NULL_ENTITY);
    m_children.erase(
        std::remove(m_children.begin(), m_children.end(), child.getId()),
        m_children.end());
}

glm::vec3 GameObject::setPosition(glm::vec3 newPosition) const
{
    return TransformSystem::setTranslation(m_transformHandle, newPosition);
}

glm::vec3 GameObject::getPosition() const
{
    return TransformSystem::getPosition(m_transformHandle);
}

glm::vec3 GameObject::move(glm::vec3 direction) const
{
    glm::vec3 currentPos = TransformSystem::getPosition(m_transformHandle);
    return TransformSystem::setTranslation(m_transformHandle, currentPos + direction);
}

glm::quat GameObject::setRotation(glm::quat newRotation) const
{
    return TransformSystem::setRotation(m_transformHandle, newRotation);
}

glm::quat GameObject::getRotation() const
{
    return TransformSystem::getRotation(m_transformHandle);
}

glm::quat GameObject::rotate(glm::quat rotation) const
{
    glm::quat currentRot = TransformSystem::getRotation(m_transformHandle);
    return TransformSystem::setRotation(m_transformHandle, rotation * currentRot);
}

glm::vec3 GameObject::setScale(glm::vec3 newScale) const
{
    return TransformSystem::setScale(m_transformHandle, newScale);
}

glm::vec3 GameObject::getScale() const
{
    return TransformSystem::getScale(m_transformHandle);
}

glm::vec3 GameObject::scale(float multiplier) const
{
    glm::vec3 currentScale = TransformSystem::getScale(m_transformHandle);
    return TransformSystem::setScale(m_transformHandle, currentScale * multiplier);
}

} // namespace dusk
