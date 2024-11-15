#pragma once

#include "entity.h"

namespace dusk
{
	class Scene
	{
	public:
		Scene();
		~Scene();

		EntityRegistry getRegistry() const
		{ return m_registry; }

		GameObject addGameObject(GameObject obj);
		void destroyGameObject(GameObject obj);

		template<typename... Components>
		auto GetGameObjectsWith()
		{
			return m_registry.view<Components...>();
		}
	private:
		EntityRegistry m_registry;
	};
}
