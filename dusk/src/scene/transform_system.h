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

    uint32_t allocate();
    void     recomputeLocal(uint32_t handle);
};

// TODO:: Pointer chasing in getter/setters, need refactoring
class TransformSystem
{
public:
    TransformSystem();
    ~TransformSystem();

    bool init(size_t maxTransformCount);
    void cleanup();
    void resrveStorageCapacity(size_t maxTransformsCount);
    void updateMatrices();
    void markDirty(uint32_t handle);

public:
    static uint32_t          create(EntityId entityId, EntityId parentId = NULL_ENTITY);
    static TransformStorage* getStorage();

    static void              setParent(EntityId id, EntityId parentId);

    static glm::vec3         setTranslation(uint32_t handle, const glm::vec3& newTranslation);
    static glm::quat         setRotation(uint32_t handle, const glm::quat& newRotation);
    static glm::vec3         setScale(uint32_t handle, const glm::vec3& newScale);

    static glm::vec3         getPosition(uint32_t handle);
    static glm::vec3         getPosition(EntityId id);
    static glm::quat         getRotation(uint32_t handle);
    static glm::quat         getRotation(EntityId id);
    static glm::vec3         getScale(uint32_t handle);
    static glm::vec3         getScale(EntityId id);

    static glm::mat4         getWorldMatrix(uint32_t handle);
    static glm::mat4         getWorldMatrix(EntityId id);
    static glm::mat4         getLocalMatrix(uint32_t handle);
    static glm::mat4         getLocalMatrix(EntityId id);
    static glm::mat4         getNormalMatrix(uint32_t handle);
    static glm::mat4         getNormalMatrix(EntityId id);
    static uint32_t          getEntityHandle(EntityId id);

    static bool              isDirty(EntityId id);

private:
    Unique<TransformStorage>               m_storage        = nullptr;

    std::unordered_map<EntityId, uint32_t> m_entityToHandle = {};
    std::unordered_map<uint32_t, EntityId> m_handleToEntity = {};

private:
    static TransformSystem* s_instance;
};

} // namespace dusk