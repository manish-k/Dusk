#include "game_object.h"

namespace dusk
{
	GameObject::GameObject(Scene& scene) : Entity(scene.getRegistry())
		: m_scene(scene)
	{

	}

	GameObject::~GameObject()
	{
		m_scene.getRegistry().remove(m_id);
	}
}
