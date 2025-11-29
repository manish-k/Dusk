#pragma once

#include "dusk.h"

#pragma warning(push, 0)
#include <entt.hpp>
#pragma warning(pop)

#define NULL_ENTITY entt::null

namespace dusk
{
using EntityId       = entt::entity;
using EntityRegistry = entt::registry;

/**
 * @brief World class for storing global items such as entity registry
 */
class Registry
{
public:
    Registry()  = default;
    ~Registry() = default;

    CLASS_UNCOPYABLE(Registry)

    /**
     * @brief Get entity-component global registry
     * @return Reference to registry object
     */
    static EntityRegistry& getRegistry() { return s_registry; };

    /**
     * @brief Clears the static s_registry container and resets it to an empty state.
     */
    static void cleanup()
    {
        s_registry.clear();
        s_registry = {};
    }

private:
    static EntityRegistry s_registry;
};
} // namespace dusk