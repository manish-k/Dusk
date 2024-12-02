#include "entity.h"

namespace dusk
{
Entity::Entity(EntityRegistry& registry) :
    m_registry { registry }
{
    m_id = m_registry.create();
}
} // namespace dusk
