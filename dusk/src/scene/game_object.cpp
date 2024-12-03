#include "game_object.h"

#include "components/transform.h"

#include <algorithm>

namespace dusk
{
GameObject::GameObject()
{
    DUSK_DEBUG("Creating GameObject");
    m_name = "New GameObject";
    addComponent<TransformComponent>();
}

GameObject::~GameObject()
{
    DUSK_DEBUG("Destroying GameObject {}", m_name);
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
} // namespace dusk
