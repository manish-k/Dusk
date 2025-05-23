#pragma once

#include "dusk.h"
#include "entity.h"

#include <unordered_map>
#include <string_view>

namespace dusk
{
class GameObject final : public Entity
{
public:
    using UMap = std::unordered_map<EntityId, Unique<GameObject>>;
    using SMap = std::unordered_map<EntityId, Shared<GameObject>>;

public:
    GameObject();
    ~GameObject() override;

    /**
     * @brief Set name of the game object
     * @param name
     */
    void setName(std::string_view name) { m_name = name; }

    /**
     * @brief Add game object as children
     * @param child game object
     */
    void addChild(GameObject& child);

    /**
     * @brief Remove child game object
     * @param child game object
     */
    void removeChild(GameObject& child);

    /**
     * @brief Get entity id of the parent
     * @return EntityId
     */
    EntityId getParentId() const { return m_parent; }

    /**
     * @brief Set parent id for a game object
     * @param parentId
     */
    void setParent(EntityId parentId) { m_parent = parentId; }

    DynamicArray<EntityId>& getChildren() { return m_children; }
    std::string            getName() { return m_name; }
    const char*             getCName() { return m_name.c_str(); }

private:
    std::string            m_name   = "GameObject";
    EntityId               m_parent = NULL_ENTITY;
    DynamicArray<EntityId> m_children {};
};
} // namespace dusk
