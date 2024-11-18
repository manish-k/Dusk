#include "scene.h"

namespace dusk
{
	Scene::Scene(std::string_view name)
		: m_name{ name }
	{
		DUSK_DEBUG("Creating scene {}", name);
		m_root = m_registry.create();
	}

	Scene::~Scene()
	{
		DUSK_DEBUG("Destroying scene {}", m_name);
		m_registry.clear();
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
	}

	GameObject& Scene::getGameObject(EntityId objectId)
	{
		DASSERT(m_sceneGameObjects.contains(objectId), "Scene does not have the given game object");
		return *m_sceneGameObjects[objectId];
	}
}
