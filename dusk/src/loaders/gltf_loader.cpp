#include "gltf_loader.h"

namespace dusk
{
	GLTFLoader::GLTFLoader()
	{

	}


	Unique<Scene> GLTFLoader::readScene(std::string_view fileName)
	{
		tinygltf::TinyGLTF loader;
		std::string loadErr;
		std::string loadWarn;
		bool loadResult;

		loadResult = loader.LoadASCIIFromFile(
			&model, &loadErr, &loadWarn, fileName.data());

		return parseScene(model.defaultScene);
	}

	Unique<Scene> GLTFLoader::parseScene(int sceneIndex)
	{
		auto scene = createUnique<Scene>("Scene");
		
		
		return scene;
	}

	Unique<TransformComponent> GLTFLoader::parseTransform(
		const tinygltf::Node& node)
	{
		auto transform = createUnique<TransformComponent>();

		// check translation data
		if (!node.translation.empty())
		{
			transform->setTranslation({
				node.translation[0],
				node.translation[1],
				node.translation[2]
				});
		}

		// check scale data
		if (!node.scale.empty())
		{
			transform->setScale({
				node.scale[0],
				node.scale[1],
				node.scale[2]
				});
		}

		// check rotation data
		if (!node.rotation.empty())
		{
			transform->setRotation(glm::quat
				{
					(float)node.rotation[0],
					(float)node.rotation[1],
					(float)node.rotation[2],
					(float)node.rotation[3]
				});
		}

		return transform;
	}
}