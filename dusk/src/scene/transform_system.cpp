#include "transform_system.h"

namespace dusk
{

TransformSystem* TransformSystem::s_instance = nullptr;

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
    m_storage->firstChild.reserve(maxTransformsCount);
    m_storage->nextSibling.reserve(maxTransformsCount);
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

TransformHandle TransformSystem::create(EntityId entityId)
{
    TransformHandle handle = s_instance->m_storage->count;
    s_instance->m_storage->count++;

    s_instance->m_entityToHandle[entityId] = handle;
    s_instance->m_handleToEntity[handle]   = entityId;

    return handle;
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