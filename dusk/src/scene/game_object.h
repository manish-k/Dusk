#pragma once

#include "entity.h"
#include "scene.h"
#include "core/base.h"

namespace dusk
{
	class GameObject final : public Entity
	{
	public:
		GameObject(Scene& scene);
		~GameObject();

	private:
		Scene& m_scene;
		Shared<GameObject> m_parent;
		std::vector<Shared<GameObject>> m_children;
	};
}
