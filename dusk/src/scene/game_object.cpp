#include "game_object.h"

#include "components/transform.h"

#include <algorithm>

namespace dusk
{
	GameObject::GameObject(EntityRegistry& registry)
		: Entity(registry)
	{
		DUSK_DEBUG("Creating GameObject");
		addComponent<TransformComponent>();
	}

	GameObject::~GameObject()
	{
		DUSK_DEBUG("Destroying GameObject");
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
}