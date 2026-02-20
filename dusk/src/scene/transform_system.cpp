#include "transform_system.h"

namespace dusk
{

TransformSystem* TransformSystem::s_instance = nullptr;

TransformHandle  TransformStorage::allocate()
{
    TransformHandle handle = count;
    ++count;

    // add defaults
    parent.push_back(0u);
    subtreeEnd.push_back(0u);
    depth.push_back(0u);

    translation.emplace_back(0.f);
    rotation.emplace_back(1.0f, 0.f, 0.f, 0.f);
    scale.emplace_back(1.0f);

    local.emplace_back(1.0f);
    world.emplace_back(1.0f);
    normal.emplace_back(1.0f);

    dirtyList.push_back(1u);

    return handle;
}

TransformSystem::TransformSystem()
{
    DASSERT(!s_instance, "Transform system's instance already exists");
    s_instance = this;
}

TransformSystem::~TransformSystem()
{
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

void TransformSystem::update()
{
}

TransformHandle TransformSystem::create(EntityId entityId, EntityId parentId)
{
    auto&           storage                = s_instance->m_storage;
    TransformHandle handle                 = storage->allocate();

    s_instance->m_entityToHandle[entityId] = handle;
    s_instance->m_handleToEntity[handle]   = entityId;

    if (parentId != NULL_ENTITY)
    {
        TransformHandle parentHandle = s_instance->m_entityToHandle[parentId];
        storage->parent[handle]      = parentHandle;
    }

    return handle;
}

TransformStorage* TransformSystem::getStorage()
{
    return s_instance->m_storage.get();
}

void TransformSystem::setTranslation(TransformHandle handle, const glm::vec3& newTranslation)
{
    s_instance->m_storage->translation[handle] = newTranslation;
    s_instance->m_storage->dirtyList[handle]   = true;
}

void TransformSystem::setRotation(TransformHandle handle, const glm::quat& newRotation)
{
    s_instance->m_storage->rotation[handle]  = newRotation;
    s_instance->m_storage->dirtyList[handle] = true;
}

void TransformSystem::setScale(TransformHandle handle, const glm::vec3& newScale)
{
    s_instance->m_storage->scale[handle]     = newScale;
    s_instance->m_storage->dirtyList[handle] = true;
}

glm::mat4 TransformSystem::getWorldMatrix(TransformHandle handle)
{
    return s_instance->m_storage->world[handle];
}

glm::mat4 TransformSystem::getLocalMatrix(TransformHandle handle)
{
    return s_instance->m_storage->local[handle];
}

glm::mat4 TransformSystem::getNormalMatrix(TransformHandle handle)
{
    return s_instance->m_storage->normal[handle];
}

} // namespace dusk