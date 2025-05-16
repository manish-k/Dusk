#include "scene.h"

#include "engine.h"
#include "camera_controller.h"
#include "events/event.h"

#include "loaders/assimp_loader.h"
#include "loaders/stb_image_loader.h"

#include "renderer/vertex.h"

#include "components/camera.h"
#include "components/mesh.h"
#include "components/transform.h"

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
    cameraTransform.translation         = { 0.f, 0.f, 5.f };

    auto& cameraComponent               = camera->addComponent<CameraComponent>();
    const auto& currentExtent           = Engine::get().getRenderer().getSwapChain().getCurrentExtent();
    cameraComponent.setPerspectiveProjection(glm::radians(50.f), static_cast<float>(currentExtent.width) / static_cast<float>(currentExtent.height), 0.5f, 1000.f);
    cameraComponent.setView(cameraTransform.translation, cameraTransform.rotation);

    m_cameraController        = createUnique<CameraController>(*camera, currentExtent.width, currentExtent.height);

    m_cameraId                = camera->getId();
    addGameObject(std::move(camera), rootId);
}

Scene::~Scene()
{
    DUSK_DEBUG("Destroying scene {}", m_name);

    freeSubMeshes();
    freeTextures();
    freeMaterials();

    Registry::getRegistry().clear();
}

void Scene::onEvent(Event& ev)
{
    m_cameraController->onEvent(ev);
}

void Scene::onUpdate(TimeStep dt)
{
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

int Scene::loadTexture(std::string& path)
{
    if (path.empty())
    {
        DUSK_ERROR("empty path for loading th texture");
        return -1;
    }

    if (!m_texPathMap.has(path))
    {
        // Texture was not loaded earlier
        auto img = ImageLoader::readImage(path);
        if (img)
        {
            uint32_t  newId = m_textures.size();
            Texture2D newTex(newId);

            Error     err = newTex.init(*img);
            if (err != Error::Ok)
            {
                return -1;
            }

            m_texPathMap.emplace(path, newId);
            m_textures.push_back(std::move(newTex));

            ImageLoader::freeImage(*img);
            return newId;
        }
        else
        {
            ImageLoader::freeImage(*img);
            return -1; // file not found
        }
    }

    // Texture has been loaded already
    return m_texPathMap[path];
}

void Scene::freeTextures()
{
    uint32_t texCount = m_textures.size();
    for (uint32_t texIndex = 0u; texIndex < texCount; ++texIndex)
    {
        m_textures[texIndex].free();
    }

    m_textures.clear();
    m_texPathMap.clear();
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

} // namespace dusk
