#pragma once

#include "game_object.h"

//include all components for others
#include "components/camera.h"
#include "components/mesh.h"
#include "components/script.h"
#include "components/transform.h"

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
		Scene(std::string_view name);
		~Scene();

		/**
		 * @brief Get entity-component registry
		 * @return Reference to registry object
		 */
		EntityRegistry& getRegistry()
		{
			return m_registry;
		};

		/**
		 * @brief Get root entity id for the scene
		 * @return EntityId
		 */
		EntityId getRootId() { return m_root; }

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
		 * @return iterator for game objects
		 */
		template<typename... Components>
		auto GetGameObjectsWith()
		{
			return m_registry.view<Components...>();
		}
	private:
		std::string_view m_name;
		EntityRegistry m_registry;
		EntityId m_root = NULL_ENTITY;
		std::vector<EntityId> m_children{};
		GameObject::UMap m_sceneGameObjects{};
	};
}
