#include "scene.h"

#include "engine.h"
#include "camera_controller.h"
#include "transform_system.h"
#include "events/event.h"

#include "loaders/assimp_loader.h"

#include "components/camera.h"
#include "components/renderable.h"

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

    const auto& currentExtent   = Engine::get().getRenderer().getSwapChain().getCurrentExtent();
    auto&       cameraComponent = camera->addComponent<CameraComponent>();
    cameraComponent.setPerspectiveProjection(glm::radians(50.f), static_cast<float>(currentExtent.width) / static_cast<float>(currentExtent.height), 0.5f, 10000.f);

    m_cameraController = createUnique<CameraController>(
        *camera,
        currentExtent.width,
        currentExtent.height,
        glm::vec3(0.f, 1.f, 0.f));

    m_cameraId = camera->getId();
    addGameObject(std::move(camera), rootId);
}

Scene::~Scene()
{
    DUSK_DEBUG("Destroying scene {}", m_name);

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

    // TODO:: this can be optimized further by maintaining a list of dirty transforms
    // update world AABBs for all mesh components whose transforms are dirty
    Registry::getRegistry().view<RenderableComponent>().each(
        [&](auto entity, auto& meshData)
        {
            if (!TransformSystem::isDirty(entity))
            {
                return;
            }

            meshData.worldAABB = recomputeAABB(meshData.objectAABB, TransformSystem::getWorldMatrix(entity));
        });
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

    TransformSystem::setParent(objectId, parentId);
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

CameraComponent& Scene::getMainCamera()
{
    return Registry::getRegistry().get<CameraComponent>(m_cameraId);
}

CameraController& Scene::getMainCameraController()
{
    return *m_cameraController;
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

void Scene::gatherRenderables(GfxRenderables* currentFrameRenderables)
{
    DUSK_PROFILE_FUNCTION;
    Registry::getRegistry().view<RenderableComponent>().each(
        [&](auto entity, auto& renderableData)
        {
            glm::vec3 center  = (renderableData.worldAABB.min + renderableData.worldAABB.max) * 0.5f;
            glm::vec3 extents = (renderableData.worldAABB.max - renderableData.worldAABB.min) * 0.5f;

            for (uint32_t index = 0u; index < renderableData.meshes.size(); ++index)
            {
                currentFrameRenderables->modelMatrices.push_back(TransformSystem::getWorldMatrix(entity));
                currentFrameRenderables->normalMatrices.push_back(TransformSystem::getNormalMatrix(entity));
                currentFrameRenderables->boundingBoxes.push_back(
                    GfxBoundingBoxData {
                        .center  = center,
                        .extents = extents,
                    });
                currentFrameRenderables->meshIds.push_back(renderableData.meshes[index]);
                currentFrameRenderables->materialIds.push_back(renderableData.materials[index]);
            }
        });
}

} // namespace dusk
