#include "scene.h"

#include "loaders/assimp_loader.h"

namespace dusk
{
Scene::Scene(const std::string_view name) :
    m_name { name }
{
    DUSK_DEBUG("Creating scene {}", name);
    auto root = createUnique<GameObject>();
    root->setName("Root");

    auto rootId = root->getId();
    m_sceneGameObjects.emplace(rootId, std::move(root));
    m_root      = rootId;

    auto camera = createUnique<GameObject>();
    camera->setName("Camera");
    auto& component = camera->addComponent<CameraComponent>();
    component.setViewTarget({ 0.f, -2.f, -5.f }, { 0.f, 0.f, 0.f }, { 0.f, -1.f, 0.f });

    m_cameraId = camera->getId();
    addGameObject(std::move(camera), rootId);
}

Scene::~Scene()
{
    DUSK_DEBUG("Destroying scene {}", m_name);

    freeSubMeshes();

    Registry::getRegistry().clear();
}

void Scene::addGameObject(
    Unique<GameObject> object, EntityId parentId)
{
    DASSERT(!m_sceneGameObjects.contains(object->getId()), "Scene already has given game object");

    DASSERT(m_sceneGameObjects.contains(parentId), "Scene does not have the given parent object");

    auto objectId = object->getId();
    m_sceneGameObjects.emplace(objectId, std::move(object));

    auto& parent = getGameObject(parentId);
    parent.addChild(getGameObject(objectId));
}

void Scene::destroyGameObject(GameObject& object)
{
    DASSERT(m_sceneGameObjects.contains(object.getId()), "Scene does not have given game object");

    // detach from parent
    auto& parent = getGameObject(object.getParentId());
    parent.removeChild(object);

    // destroy
    auto objectId = object.getId();
    m_sceneGameObjects.erase(objectId);
    Registry::getRegistry().destroy(objectId);
}

GameObject& Scene::getGameObject(EntityId objectId)
{
    DASSERT(m_sceneGameObjects.contains(objectId), "Scene does not have the given game object");
    return *m_sceneGameObjects[objectId];
}

void Scene::initSubMesh(uint32_t meshIndex, const DynamicArray<Vertex>& vertices, const DynamicArray<uint32_t> indices)
{
    DASSERT(meshIndex >= 0, "mesh index can't be negative");
    DASSERT(meshIndex < m_subMeshes.size(), "mesh index can't be greater than total sub meshes");

    m_subMeshes[meshIndex].init(vertices, indices);
}

void Scene::freeSubMeshes()
{
    for (uint32_t meshIndex = 0u; meshIndex < m_subMeshes.size(); ++meshIndex)
    {
        m_subMeshes[meshIndex].free();
    }
}

CameraComponent& Scene::getMainCamera()
{
    return Registry::getRegistry().get<CameraComponent>(m_cameraId);
}

Unique<Scene> Scene::createSceneFromGLTF(const std::string& fileName)
{
    AssimpLoader loader {};
    return loader.readScene(fileName);
}
} // namespace dusk
