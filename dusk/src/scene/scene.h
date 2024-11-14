#pragma once

#include "entt.hpp"

namespace dusk
{
	class Scene
	{
	public:
		Scene();
		~Scene();

		entt::registry getRegistry() const 
		{ return m_registry; }

	private:
		entt::registry m_registry;
	};
}
