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

    /**
     * @brief Get children list of the game object
     * @return Array of children ids
     */
    DynamicArray<EntityId>& getChildren() { return m_children; }

    /**
     * @brief Get name of the game object
     * @return Name string
     */
    std::string getName() const { return m_name; }

    /**
     * @brief Get name of the game object
     * @return Name c string
     */
    const char* getCName() const { return m_name.c_str(); }

    /**
     * @brief Get transform handle of the game object
     * @return Transform handle
     */
    uint32_t getTransformHandle() const { return m_transformHandle; }

    /**
     * @brief Set position of the game object
     * @param New position vector
     * @return new position vector
     */
    glm::vec3 setPosition(glm::vec3 newPosition) const;

    /**
     * @brief Get position of the game object
     * @return Current position vector
     */
    glm::vec3 getPosition() const;

    /**
     * @brief Move game object in a given direction
     * @param direction vector
     * @return New position of the game object
     */
    glm::vec3 move(glm::vec3 direction) const;

    /**
     * @brief Get rotation of the game object
     * @return Rotation quaternion
     */
    glm::quat getRotation() const;

    /**
     * @brief Set rotation of the game object
     * @param rotation quaternion
     * @return Current rotation quaternion
     */
    glm::quat setRotation(glm::quat newRotation) const;

    /**
     * @brief Rotate the game object by given rotation
     * @param Rotation quaternion
     * @return New rotation of the game object
     */
    glm::quat rotate(glm::quat rotation) const;

    /**
     * @brief Get current scale of the game object
     * @return Scale vector
     */
    glm::vec3 getScale() const;

    /**
     * @brief Set scale of the game object
     * @param New scale vector
     * @return Scale vector
     */
    glm::vec3 setScale(glm::vec3 newScale) const;

    /**
     * @brief Scale the game object by given multiplier
     * @param mulitplier
     * @return New scale of the game object
     */
    glm::vec3 scale(float mulitplier) const;

private:
    std::string            m_name   = "GameObject";
    EntityId               m_parent = NULL_ENTITY;
    DynamicArray<EntityId> m_children {};

    uint32_t               m_transformHandle = 0u;
};
} // namespace dusk
