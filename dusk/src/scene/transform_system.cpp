#include "transform_system.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace dusk
{

TransformSystem* TransformSystem::s_instance = nullptr;

uint32_t         TransformStorage::allocate()
{
    uint32_t handle = count;
    ++count;

    // add defaults
    parent.push_back(0u);
    subtreeEnd.push_back(0u);

    translation.emplace_back(0.f);
    rotation.emplace_back(1.0f, 0.f, 0.f, 0.f);
    scale.emplace_back(1.0f);

    local.emplace_back(1.0f);
    world.emplace_back(1.0f);
    normal.emplace_back(1.0f);

    dirtyList.push_back(1u);

    return handle;
}

void TransformStorage::recomputeLocal(uint32_t handle)
{
    const auto& t = translation[handle];
    const auto& r = rotation[handle];
    const auto& s = scale[handle];

    local[handle] = glm::mat4 {
        {
            s.x * (1 - 2 * (r.y * r.y + r.z * r.z)),
            s.x * (2 * (r.x * r.y + r.w * r.z)),
            s.x * (2 * (r.x * r.z - r.w * r.y)),
            0.0f,
        },
        {
            s.y * (2 * (r.x * r.y - r.w * r.z)),
            s.y * (1 - 2 * (r.x * r.x + r.z * r.z)),
            s.y * (2 * (r.y * r.z + r.w * r.x)),
            0.0f,
        },
        {
            s.z * (2 * (r.x * r.z + r.w * r.y)),
            s.z * (2 * (r.y * r.z - r.w * r.x)),
            s.z * (1 - 2 * (r.y * r.y + r.x * r.x)),
            0.0f,
        },
        { t.x, t.y, t.z, 1.0f }
    };

    normal[handle] = glm::transpose(glm::inverse(glm::mat3(local[handle])));
}

TransformSystem::TransformSystem()
{
    DASSERT(!s_instance, "Transform system's instance already exists");
    s_instance = this;
}

TransformSystem::~TransformSystem()
{
    m_storage = nullptr;
}

bool TransformSystem::init(size_t maxTransformsCount)
{
    m_storage        = createUnique<TransformStorage>();

    m_storage->count = 0u;
    resrveStorageCapacity(maxTransformsCount);

    return true;
}

void TransformSystem::cleanup()
{
    m_storage = nullptr;

    m_entityToHandle.clear();
    m_handleToEntity.clear();
}

void TransformSystem::resrveStorageCapacity(size_t maxTransformsCount)
{
    m_storage->parent.reserve(maxTransformsCount);
    m_storage->translation.reserve(maxTransformsCount);
    m_storage->rotation.reserve(maxTransformsCount);
    m_storage->scale.reserve(maxTransformsCount);
    m_storage->local.reserve(maxTransformsCount);
    m_storage->world.reserve(maxTransformsCount);
    m_storage->normal.reserve(maxTransformsCount);
    m_storage->dirtyList.reserve(maxTransformsCount);
}

void TransformSystem::updateDirtyMatrices()
{
    uint32_t transformsCount = m_storage->count;
    for (uint32_t handle = 0u; handle < transformsCount; ++handle)
    {
        if (!m_storage->dirtyList[handle])
        {
            continue;
        }

        // recompute new local
        m_storage->recomputeLocal(handle);

        uint32_t parent              = m_storage->parent[handle];
        m_storage->world[handle]     = m_storage->world[parent] * m_storage->local[handle];

        m_storage->dirtyList[handle] = 0u;
    }
}

void TransformSystem::markDirty(uint32_t handle)
{
    m_storage->dirtyList[handle] = 1u;

    // mark all children
    for (uint32_t childHandle = handle + 1; childHandle <= m_storage->subtreeEnd[handle]; ++childHandle)
    {
        m_storage->dirtyList[childHandle] = 1u;
    }
}

uint32_t TransformSystem::create(EntityId entityId, EntityId parentId)
{
    const auto& storage                    = s_instance->m_storage;
    uint32_t    handle                     = storage->allocate();

    s_instance->m_entityToHandle[entityId] = handle;
    s_instance->m_handleToEntity[handle]   = entityId;

    if (parentId != NULL_ENTITY)
    {
        uint32_t parentHandle   = s_instance->m_entityToHandle[parentId];
        storage->parent[handle] = parentHandle;
    }

    storage->subtreeEnd[handle] = handle;

    return handle;
}

TransformStorage* TransformSystem::getStorage()
{
    return s_instance->m_storage.get();
}

void TransformSystem::setParent(EntityId id, EntityId parentId)
{
    auto handle       = s_instance->m_entityToHandle[id];
    auto parentHandle = s_instance->m_entityToHandle[parentId];

    // set and mark dirty
    s_instance->m_storage->parent[handle] = parentHandle;
    s_instance->markDirty(handle);
}

glm::vec3 TransformSystem::setTranslation(uint32_t handle, const glm::vec3& newTranslation)
{
    s_instance->m_storage->translation[handle] = newTranslation;
    s_instance->markDirty(handle);

    return s_instance->m_storage->translation[handle];
}

glm::quat TransformSystem::setRotation(uint32_t handle, const glm::quat& newRotation)
{
    s_instance->m_storage->rotation[handle] = newRotation;
    s_instance->markDirty(handle);

    return s_instance->m_storage->rotation[handle];
}

glm::vec3 TransformSystem::setScale(uint32_t handle, const glm::vec3& newScale)
{
    s_instance->m_storage->scale[handle] = newScale;
    s_instance->markDirty(handle);

    return s_instance->m_storage->scale[handle];
}

glm::vec3 TransformSystem::getPosition(uint32_t handle)
{
    return s_instance->m_storage->translation[handle];
}

glm::vec3 TransformSystem::getPosition(EntityId id)
{
    auto handle = s_instance->m_entityToHandle[id];
    return s_instance->m_storage->translation[handle];
}

glm::quat TransformSystem::getRotation(uint32_t handle)
{
    return s_instance->m_storage->rotation[handle];
}

glm::quat TransformSystem::getRotation(EntityId id)
{
    auto handle = s_instance->m_entityToHandle[id];
    return s_instance->m_storage->rotation[handle];
}

glm::vec3 TransformSystem::getScale(uint32_t handle)
{
    return s_instance->m_storage->scale[handle];
}

glm::vec3 TransformSystem::getScale(EntityId id)
{
    auto handle = s_instance->m_entityToHandle[id];
    return s_instance->m_storage->scale[handle];
}

glm::mat4 TransformSystem::getWorldMatrix(uint32_t handle)
{
    return s_instance->m_storage->world[handle];
}

glm::mat4 TransformSystem::getWorldMatrix(EntityId id)
{
    auto handle = s_instance->m_entityToHandle[id];
    return s_instance->m_storage->world[handle];
}

glm::mat4 TransformSystem::getLocalMatrix(uint32_t handle)
{
    return s_instance->m_storage->local[handle];
}

glm::mat4 TransformSystem::getLocalMatrix(EntityId id)
{
    auto handle = s_instance->m_entityToHandle[id];
    return s_instance->m_storage->local[handle];
}

glm::mat4 TransformSystem::getNormalMatrix(uint32_t handle)
{
    return s_instance->m_storage->normal[handle];
}

glm::mat4 TransformSystem::getNormalMatrix(EntityId id)
{
    auto handle = s_instance->m_entityToHandle[id];
    return s_instance->m_storage->normal[handle];
}

uint32_t TransformSystem::getEntityHandle(EntityId id)
{
    return s_instance->m_entityToHandle[id];
}

bool TransformSystem::isDirty(EntityId id)
{
    auto handle = s_instance->m_entityToHandle[id];
    return s_instance->m_storage->dirtyList[handle];
}

} // namespace dusk