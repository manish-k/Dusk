#pragma once

#include "game_object.h"
#include "registry.h"

// include all components for other files
#include "components/camera.h"
#include "components/mesh.h"
#include "components/script.h"
#include "components/transform.h"

#include "renderer/sub_mesh.h"

#include <string>

namespace dusk
{
class Scene
{
public:
    /**
     * @brief Create scene with a given name
     * @param name of the scene
     */
    Scene(const std::string_view name);
    ~Scene();

    CLASS_UNCOPYABLE(Scene)

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

    void initMeshes(uint32_t meshCount)
    {
        m_subMeshes = DynamicArray<SubMesh>(meshCount);
    }

    /**
     * @brief
     */
    void initSubMesh(uint32_t meshIndex, const DynamicArray<Vertex>& vertices, const DynamicArray<uint32_t> indices);

    /**
     * @brief
     */
    void             freeSubMeshes();

    SubMesh&         getSubMesh(uint32_t meshId) { return m_subMeshes[meshId]; }

    CameraComponent& getMainCamera();

    /**
     * @brief Create a scene from a gltf file
     * @param fileName
     * @return unique pointer to the scene object
     */
    static Unique<Scene> createSceneFromGLTF(
        const std::string& fileName);

private:
    const std::string      m_name;
    EntityId               m_root = NULL_ENTITY;
    DynamicArray<EntityId> m_children {};
    GameObject::UMap       m_sceneGameObjects {};
    DynamicArray<SubMesh>  m_subMeshes {};
    EntityId               m_cameraId;
};
} // namespace dusk
