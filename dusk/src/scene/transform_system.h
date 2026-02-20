#pragma once

#include "dusk.h"

#include "registry.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

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
};

class TransformSystem
{
public:
    TransformSystem();
    ~TransformSystem();

    bool init(size_t maxTransformCount);
    void cleanup();
    void resrveStorageCapacity(size_t maxTransformsCount);
    void update();

public:
    static TransformHandle   create(EntityId entityId, EntityId parentId = NULL_ENTITY);
    static TransformStorage* getStorage();

    static void              setTranslation(TransformHandle handle, const glm::vec3& newTranslation);
    static void              setRotation(TransformHandle handle, const glm::quat& newRotation);
    static void              setScale(TransformHandle handle, const glm::vec3& newScale);

    static glm::mat4         getWorldMatrix(TransformHandle handle);
    static glm::mat4         getLocalMatrix(TransformHandle handle);
    static glm::mat4         getNormalMatrix(TransformHandle handle);

private:
    Unique<TransformStorage>                      m_storage        = nullptr;

    std::unordered_map<EntityId, TransformHandle> m_entityToHandle = {};
    std::unordered_map<TransformHandle, EntityId> m_handleToEntity = {};

private:
    static TransformSystem* s_instance;
};

} // namespace dusk