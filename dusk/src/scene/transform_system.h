#pragma once

#include "dusk.h"

#include "registry.h"

#include <glm/glm.hpp>

// Note: Nodes should be added in DFS order to ensure parent id < children id. This helps in
// tracking end point of subtrees which helps in rejecting non-dirty nodes and dirty propogation
// in subtree range

// TODO: consider using exclusive allocated single memory block for transform system

namespace dusk
{
struct TransformStorage
{
    CLASS_UNCOPYABLE(TransformStorage);

    // total transforms
    uint32_t count = 0u;

    // transforms hierarchy
    DynamicArray<uint32_t> parent;
    DynamicArray<uint32_t> subtreeEnd;
    DynamicArray<uint16_t> depth;

    // transform data
    DynamicArray<glm::vec3> translation;
    DynamicArray<glm::quat> rotation;
    DynamicArray<glm::vec3> scale;

    // transform matrices
    DynamicArray<glm::mat4> local;
    DynamicArray<glm::mat4> world;
    DynamicArray<glm::mat4> normal;

    // TODO:: use dynamic bitset
    DynamicArray<uint8_t> dirtyList;

    TransformStorage() = default;
    /**
     * @brief Allocate a new transform
     * @return Handle of allocated transform
     */
    uint32_t allocate();

    /**
     * @brief Recompute local transform matrix based on translation, rotation and
     * scale values
     * @param Handle of transform
     */
    void recomputeLocal(uint32_t handle);
};

// TODO:: Pointer chasing in getter/setters, need refactoring
class TransformSystem
{
public:
    TransformSystem();
    ~TransformSystem();

    /**
     * @brief Initialize the transform system and storage
     * @param Total transforms to support
     * @return true if init successful else false
     */
    bool init(size_t maxTransformCount);

    /**
     * @brief Clean transform storage
     */
    void cleanup();

    /**
     * @brief Reserve storage capacity of transform storage
     * @param total transform for which storage should be reserved
     */
    void resrveStorageCapacity(size_t maxTransformsCount);

    /**
     * @brief Update dirty transforms matrices
     */
    void updateDirtyMatrices();

    /**
     * @brief Mark transform and its subtree as dirty
     * @param Handle of the transform
     */
    void markDirty(uint32_t handle);

    /**
     * @brief Allocate transform in the storage
     * @param Entity id for which storage should be allocated
     * @param Parent's entity id
     * @return Handle for the transform
     */
    static uint32_t create(EntityId entityId, EntityId parentId = NULL_ENTITY);

    /**
     * @brief Get underlying storage for direct access
     * @return Transform storage's pointer
     */
    static TransformStorage* getStorage();

    /**
     * @brief Set parent for the given entity
     * @param id
     * @param parent id
     */
    static void setParent(EntityId id, EntityId parentId);

    /**
     * @brief Set translation for given hanle
     * @param Handle
     * @param New translation vector
     * @return Updated translation vector
     */
    static glm::vec3 setTranslation(uint32_t handle, const glm::vec3& newTranslation);

    /**
     * @brief Set rotation for given hanle
     * @param Handle
     * @param New rotation quaternion
     * @return Updated rotation quaternion
     */
    static glm::quat setRotation(uint32_t handle, const glm::quat& newRotation);

    /**
     * @brief Set scale for given hanle
     * @param Handle
     * @param New scale vector
     * @return Updated scale vector
     */
    static glm::vec3 setScale(uint32_t handle, const glm::vec3& newScale);

    /**
     * @brief Get position for given handle
     * @param Handle
     * @return Position vector
     */
    static glm::vec3 getPosition(uint32_t handle);

    /**
     * @brief Get position for given entity
     * @param Entity Id
     * @return Position vector
     */
    static glm::vec3 getPosition(EntityId id);

    /**
     * @brief Get rotaion for given handle
     * @param Handle
     * @return Rotation quaternion
     */
    static glm::quat getRotation(uint32_t handle);

    /**
     * @brief Get rotation for given entity id
     * @param Entity id
     * @return Rotation quaternion
     */
    static glm::quat getRotation(EntityId id);

    /**
     * @brief Get scale for given handle
     * @param Handle
     * @return Scale vector
     */
    static glm::vec3 getScale(uint32_t handle);

    /**
     * @brief Get scale for given entity id
     * @param Entity id
     * @return Scale vector
     */
    static glm::vec3 getScale(EntityId id);

    /**
     * @brief Get world transform matrix for given transform handle
     * @param Handle
     * @return World space transform matrix
     */
    static glm::mat4 getWorldMatrix(uint32_t handle);

    /**
     * @brief Get world transform matrix for given entity
     * @param Entity id
     * @return World space transform matrix
     */
    static glm::mat4 getWorldMatrix(EntityId id);

    /**
     * @brief Get local transform matrix for given transform handle
     * @param Handle
     * @return Local space transform matrix
     */
    static glm::mat4 getLocalMatrix(uint32_t handle);

    /**
     * @brief Get local transform matrix for given entity
     * @param Entity id
     * @return Local space transform matrix
     */
    static glm::mat4 getLocalMatrix(EntityId id);

    /**
     * @brief Get normal transform matrix for given transform handle
     * @param Handle
     * @return Normal transform matrix
     */
    static glm::mat4 getNormalMatrix(uint32_t handle);

    /**
     * @brief Get normal transform matrix for given entity
     * @param Entity id
     * @return Normal transform matrix
     */
    static glm::mat4 getNormalMatrix(EntityId id);

    /**
     * @brief Get transform handle for a given entity
     * @param Entity id
     * @return Transform handle
     */
    static uint32_t getEntityHandle(EntityId id);

    /**
     * @brief Check if given entity's transform is dirty or not
     * @param Entity id
     * @return True if entity's transform is dirty else false
     */
    static bool isDirty(EntityId id);

private:
    Unique<TransformStorage>               m_storage        = nullptr;

    std::unordered_map<EntityId, uint32_t> m_entityToHandle = {};
    std::unordered_map<uint32_t, EntityId> m_handleToEntity = {};

private:
    static TransformSystem* s_instance;
};

} // namespace dusk