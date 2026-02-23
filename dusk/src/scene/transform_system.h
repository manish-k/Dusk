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

using TransformHandle = uint32_t;

struct TransformStorage
{
    CLASS_UNCOPYABLE(TransformStorage);

    // total transforms
    uint32_t count;

    // transforms hierarchy
    DynamicArray<TransformHandle> parent;
    DynamicArray<TransformHandle> subtreeEnd;
    DynamicArray<uint16_t>        depth;

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

    TransformHandle allocate();
    void            recomputeLocal(TransformHandle handle);
};

class TransformSystem
{
public:
    TransformSystem();
    ~TransformSystem();

    bool init(size_t maxTransformCount);
    void cleanup();
    void resrveStorageCapacity(size_t maxTransformsCount);
    void updateMatrices();
    void markDirty(TransformHandle handle);

public:
    static TransformHandle   create(EntityId entityId, EntityId parentId = NULL_ENTITY);
    static TransformStorage* getStorage();

    static void              setParent(EntityId id, EntityId parentId);

    static glm::vec3         setTranslation(TransformHandle handle, const glm::vec3& newTranslation);
    static glm::quat         setRotation(TransformHandle handle, const glm::quat& newRotation);
    static glm::vec3         setScale(TransformHandle handle, const glm::vec3& newScale);

    static glm::vec3         getPosition(TransformHandle handle);
    static glm::vec3         getPosition(EntityId id);
    static glm::quat         getRotation(TransformHandle handle);
    static glm::quat         getRotation(EntityId id);
    static glm::vec3         getScale(TransformHandle handle);
    static glm::vec3         getScale(EntityId id);

    static glm::mat4         getWorldMatrix(TransformHandle handle);
    static glm::mat4         getWorldMatrix(EntityId id);
    static glm::mat4         getLocalMatrix(TransformHandle handle);
    static glm::mat4         getLocalMatrix(EntityId id);
    static glm::mat4         getNormalMatrix(TransformHandle handle);
    static glm::mat4         getNormalMatrix(EntityId id);
    static TransformHandle   getEntityHandle(EntityId id);

private:
    Unique<TransformStorage>                      m_storage        = nullptr;

    std::unordered_map<EntityId, TransformHandle> m_entityToHandle = {};
    std::unordered_map<TransformHandle, EntityId> m_handleToEntity = {};

private:
    static TransformSystem* s_instance;
};

} // namespace dusk