#pragma once

#include "entity.h"
#include "scene.h"

namespace dusk
{
	class GameObject final : public Entity
	{
	public:
		GameObject(Scene& scene);
		~GameObject();

	private:
		Scene& m_scene;
		GameObject& m_parent;
		std::vector<GameObject> m_childrens;
	};
}
