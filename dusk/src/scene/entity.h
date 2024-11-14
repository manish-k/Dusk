#pragma once

#include "scene.h"
#include "dusk.h"

#include "entt.hpp"

namespace dusk
{
	class Entity
	{
	public:
		Entity(entt::registry& registry);
		virtual ~Entity();


		template<typename T, typename... Args>
		T& addComponent(Args&&... args)
		{
			DASSERT(!hasComponent<T>(), "Component already exist for the Entity");
			return m_registry.emplace<T>(m_id, std::forward(args)...);
		}

		template<typename T>
		T& getComponent()
		{
			DASSERT(hasComponent<T>(), "Component does not exist for the Entity");
			return m_registry.get<T>(m_id);
		}

		template<typename... Types>
		bool hasComponent()
		{
			return m_registry.all_of<Types...>(m_id);
		}

		template<typename T>
		void removeComponent()
		{
			DASSERT(hasComponent<T>(), "Component does not exist for the Entity");
			m_registry.remove<T>(m_id);
		}

	private:
		entt::entity m_id{ entt::null };
		entt::registry& m_registry;
	};
}
