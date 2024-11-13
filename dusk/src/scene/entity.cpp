#include "entity.h"

namespace dusk
{
	Entity::Entity(entt::registry& registry)
		: m_registry(registry)
	{
		m_id = registry.create();
	}
}
