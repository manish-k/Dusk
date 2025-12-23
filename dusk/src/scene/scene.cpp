#include "scene.h"

#include "engine.h"
#include "camera_controller.h"
#include "events/event.h"

#include "loaders/assimp_loader.h"

#include "renderer/vertex.h"

#include "components/camera.h"
#include "components/transform.h"
#include "components/mesh.h"

#include "debug/profiler.h"

#include "backend/vulkan/vk_renderer.h"

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

    TransformComponent& cameraTransform = camera->getComponent<TransformComponent>();
    auto&               cameraComponent = camera->addComponent<CameraComponent>();
    const auto&         currentExtent   = Engine::get().getRenderer().getSwapChain().getCurrentExtent();
    cameraComponent.setPerspectiveProjection(glm::radians(50.f), static_cast<float>(currentExtent.width) / static_cast<float>(currentExtent.height), 0.5f, 10000.f);
    cameraComponent.setView(cameraTransform.translation, cameraTransform.rotation);

    m_cameraController = createUnique<CameraController>(*camera, currentExtent.width, currentExtent.height, glm::vec3(0.f, 1.f, 0.f));

    m_cameraId         = camera->getId();
    addGameObject(std::move(camera), rootId);
}

Scene::~Scene()
{
    DUSK_DEBUG("Destroying scene {}", m_name);

    freeSubMeshes();
    freeMaterials();

    Registry::getRegistry().clear();
}

void Scene::onEvent(Event& ev)
{
    m_cameraController->onEvent(ev);
}

void Scene::onUpdate(TimeStep dt)
{
    DUSK_PROFILE_FUNCTION;
    m_cameraController->onUpdate(dt);
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
        m_subMeshes[meshIndex].cleanup();
    }
}

CameraComponent& Scene::getMainCamera()
{
    return Registry::getRegistry().get<CameraComponent>(m_cameraId);
}

CameraController& Scene::getMainCameraController()
{
    return *m_cameraController;
}

TransformComponent& Scene::getMainCameraTransform()
{
    return Registry::getRegistry().get<TransformComponent>(m_cameraId);
}

Unique<Scene> Scene::createSceneFromGLTF(const std::string& fileName)
{
    DUSK_PROFILE_FUNCTION;

    AssimpLoader loader {};
    return loader.readScene(fileName);
}

void Scene::addMaterial(Material& mat)
{
    mat.id = m_materials.size();
    m_materials.push_back(std::move(mat));
}

void Scene::freeMaterials()
{
    m_materials.clear();
}

void Scene::updateModelsBuffer(GfxBuffer& modelBuffer)
{
    DUSK_PROFILE_FUNCTION;
    auto entities = GetGameObjectsWith<TransformComponent>();
    for (auto& entity : entities)
    {
        auto  objectId  = static_cast<entt::id_type>(entity);
        auto& transform = entities.get<TransformComponent>(entity);

        {
            DUSK_PROFILE_SECTION("Update model buffer");
            ModelData md { transform.mat4(), transform.normalMat4() };
            modelBuffer.writeAndFlushAtIndex(objectId, &md, sizeof(ModelData));
        }

        if (!transform.dirty)
        {
            continue;
        }

        // update AABBs for model
        if (transform.dirty && Registry::getRegistry().all_of<MeshComponent>(entity))
        {
            auto& meshComponent     = Registry::getRegistry().get<MeshComponent>(entity);
            meshComponent.worldAABB = recomputeAABB(
                meshComponent.objectAABB,
                transform.mat4());
        }

        transform.markClean();
    }
}

} // namespace dusk
