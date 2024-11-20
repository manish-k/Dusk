#include "gltf_loader.h"

namespace dusk
{
	Unique<Scene> GLTFLoader::readScene(
		std::string_view fileName, int sceneIndex)
	{

	}

	Unique<GameObject> GLTFLoader::parseNode(const tinygltf::Node& node)
	{
		auto gameObject = createUnique<GameObject>();

		auto transform = gameObject->getComponent<TransformComponent>();

		// check translation data
		if (!node.translation.empty())
		{
			glm::vec3 newTranslation;

			std::transform(
				node.translation.begin(), 
				node.translation.end(), 
				glm::value_ptr(newTranslation),
				TypeCast<double, float>{});

			transform.setTranslation(newTranslation);
		}

		// check scale data
		if (!node.scale.empty())
		{
			glm::vec3 newScale;

			std::transform(
				node.scale.begin(),
				node.scale.end(),
				glm::value_ptr(newScale),
				TypeCast<double, float>{});

			transform.setScale(newScale);
		}

		// check rotation data
		if (!node.rotation.empty())
		{
			glm::vec3 newRotation;

			std::transform(
				node.rotation.begin(),
				node.rotation.end(),
				glm::value_ptr(newRotation),
				TypeCast<double, float>{});

			transform.setRotation(newRotation);
		}

		return gameObject;
	}
}