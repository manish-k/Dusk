#pragma once

#include "entity.h"
#include "scene.h"
#include "core/base.h"

namespace dusk
{
	class GameObject final : public Entity
	{
	public:
		GameObject(Scene& scene) : Entity(scene.getRegistry());
		~GameObject();

	private:
		Scene& m_scene;
		Shared<GameObject> m_parent = nullptr;
		std::vector<Shared<GameObject>> m_children{};
	};
}
