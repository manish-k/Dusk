#pragma once

#include "dusk.h"
#include "game_object.h"
#include "registry.h"

#include "renderer/texture.h"
#include "renderer/material.h"
#include "renderer/gfx_types.h"

#include <string>

namespace dusk
{

class CameraComponent;
class CameraController;
class TransformComponent;
class Event;

struct Vertex;
struct GfxRenderables;

class Scene
{
public:
    /**
     * @brief Create scene with a given name
     * @param name of the scene
     */
    Scene(const std::string_view name);
    ~Scene();

    CLASS_UNCOPYABLE(Scene);

    /**
     * @brief Handle incoming events;
     * @param event
     */
    void onEvent(Event& ev);

    /**
     * @brief update loop of the scene
     * @param dt time since last frame
     */
    void onUpdate(TimeStep dt);

    /**
     * @brief Get root entity id for the scene
     * @return EntityId
     */
    EntityId getRootId() const { return m_root; }

    /**
     * @brief Add game object in the scene
     * @param object unique ptr of the game object
     * @param parentId of the game object
     */
    void addGameObject(
        Unique<GameObject> object, EntityId parentId);

    /**
     * @brief Destroy game object in the scene
     * @param object
     */
    void destroyGameObject(GameObject& object);

    /**
     * @brief
     * @param objectId
     * @return
     */
    GameObject& getGameObject(EntityId objectId);

    /**
     * @brief Get all the game objects for given components
     * @tparam ...Components
     * @return iterator for entities
     */
    template <typename... Components>
    auto GetGameObjectsWith()
    {
        return Registry::getRegistry().view<Components...>();
    }

    CameraComponent&    getMainCamera();

    CameraController&   getMainCameraController();

    TransformComponent& getMainCameraTransform();

    void                initMaterialCache(uint32_t materialCount)
    {
        m_materials.reserve(materialCount);
    };

    void                    addMaterial(Material& mat);

    Material&               getMaterial(uint32_t matId) { return m_materials[matId]; };

    void                    freeMaterials();

    DynamicArray<Material>& getMaterials() { return m_materials; }
    DynamicArray<EntityId>& getChildren() { return m_children; }

    void                    gatherRenderables(GfxRenderables* currentFrameRenderables);

    /**
     * @brief Create a scene from a gltf file
     * @param fileName
     * @return unique pointer to the scene object
     */
    static Unique<Scene> createSceneFromGLTF(
        const std::string& fileName);

private:
    const std::string        m_name;
    EntityId                 m_root = NULL_ENTITY;
    DynamicArray<EntityId>   m_children {};
    GameObject::UMap         m_sceneGameObjects {};

    EntityId                 m_cameraId;
    Unique<CameraController> m_cameraController;

    DynamicArray<Material>   m_materials;

public:
    // TODO:: figure out a good system to manage scene meshes
    DynamicArray<GfxMeshData> m_sceneMeshes = {};
};
} // namespace dusk
