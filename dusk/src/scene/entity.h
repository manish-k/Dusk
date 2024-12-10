#pragma once

#include "dusk.h"
#include "registry.h"

namespace dusk
{
class Entity
{
public:
    explicit Entity();
    virtual ~Entity() = default;

    /**
     * @brief Get the unique id of the Entity
     * @return id of the entity
     */
    EntityId getId() const { return m_id; }

    /**
     * @brief Add a component struct to the entity
     * @tparam T Component type
     * @tparam ...Args
     * @param ...args args to initialize component struct
     * @return Reference to the component
     */
    template <typename T, typename... Args>
    T& addComponent(Args&&... args)
    {
        DASSERT(!hasComponent<T>(), "Component already exist for the Entity");
        return Registry::getRegistry().emplace<T>(
            m_id, std::forward<Args>(args)...);
    }

    /**
     * @brief Get the component from the entity
     * @tparam T Type of the component
     * @return Reference to the component of the entity
     */
    template <typename T>
    T& getComponent()
    {
        DASSERT(hasComponent<T>(), "Component does not exist for the Entity");
        return Registry::getRegistry().get<T>(m_id);
    }

    /**
     * @brief Check if entity has components
     * @tparam ...Types Components to check
     * @return true if all components are present, false if atleast one is absent
     */
    template <typename... Types>
    bool hasComponent()
    {
        return Registry::getRegistry().all_of<Types...>(m_id);
    }

    /**
     * @brief Remove a componet from the entity
     * @tparam T Component to remove from the entity
     */
    template <typename T>
    void removeComponent()
    {
        DASSERT(hasComponent<T>(), "Component does not exist for the Entity");
        Registry::getRegistry().remove<T>(m_id);
    }

private:
    EntityId m_id { NULL_ENTITY };
};
} // namespace dusk
