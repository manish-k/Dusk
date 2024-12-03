#include "entity.h"

namespace dusk
{
Entity::Entity()
{
    m_id = Registry::getRegistry().create();
}
} // namespace dusk
