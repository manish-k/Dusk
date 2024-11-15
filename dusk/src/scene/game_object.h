#pragma once

#include "entity.h"
#include "scene.h"
#include "core/base.h"

#include <unordered_map>

namespace dusk
{
	class GameObject final : public Entity
	{
	public:
		using UMap = std::unordered_map<EntityId, Unique<GameObject>>;
		using SMap = std::unordered_map<EntityId, Shared<GameObject>>;

	public:
		GameObject(Scene& scene) : Entity(scene.getRegistry());
		~GameObject();

		void addChild(GameObject& child);
		void removeChild();
		void setParent();

	private:
		Scene& m_scene;
		Shared<GameObject> m_parent = nullptr;
		std::vector<Shared<GameObject>> m_children{};
	};
}
