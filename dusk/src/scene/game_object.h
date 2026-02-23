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
    void                    setParent(EntityId parentId) { m_parent = parentId; }
    DynamicArray<EntityId>& getChildren() { return m_children; }

    void                    setTransformHandle(uint32_t handle) { m_transformHandle = handle; };
    uint32_t                getTransformHandle() const { return m_transformHandle; };

    std::string             getName() { return m_name; }
    const char*             getCName() { return m_name.c_str(); }

    glm::vec3               setPosition(glm::vec3 newPosition);
    glm::vec3               getPosition();
    glm::vec3               move(glm::vec3 direction);

    glm::quat               getRotation();
    glm::quat               setRotation(glm::quat newRotation);
    glm::quat               rotate(glm::quat rotation);

    glm::vec3               getScale();
    glm::vec3               setScale(glm::vec3 newScale);
    glm::vec3               scale(float mulitplier);

private:
    std::string            m_name   = "GameObject";
    EntityId               m_parent = NULL_ENTITY;
    DynamicArray<EntityId> m_children {};

    uint32_t               m_transformHandle = 0u;
};
} // namespace dusk
