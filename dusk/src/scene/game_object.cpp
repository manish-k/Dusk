#include "game_object.h"

#include "transform.h"

namespace dusk
{
	GameObject::GameObject(Scene& scene) : Entity(scene.getRegistry())
		: m_scene(scene)
	{
		addComponent<TransformComponent>(m_id);
	}

	GameObject::~GameObject()
	{
		m_scene.getRegistry().remove(m_id);
	}
}
