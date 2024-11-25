#include "gltf_loader.h"

namespace dusk
{
	GLTFLoader::GLTFLoader()
	{
		int* a = nullptr;
	}


	Unique<Scene> GLTFLoader::readScene(std::string_view fileName)
	{
		tinygltf::TinyGLTF loader;
		std::string loadErr;
		std::string loadWarn;
		bool loadResult;

		loadResult = loader.LoadASCIIFromFile(
			&m_model, &loadErr, &loadWarn, fileName.data());

		if (loadErr.size() > 0)
		{
			DUSK_ERROR("Error in reading scene. {}", loadErr);
			return createUnique<Scene>("Scene");
		}

		if (loadWarn.size() > 0)
		{
			DUSK_WARN("{}", loadWarn);
		}

		int sceneIndex = 0;
		if (m_model.scenes.size() == 0)
		{
			DUSK_CRITICAL("No scenes present in the file");
			return createUnique<Scene>("Scene");
		}
		else if (m_model.defaultScene >= 0 
			&& m_model.defaultScene < m_model.scenes.size())
		{
			sceneIndex = m_model.defaultScene;
		}

		return parseScene(sceneIndex);
	}

	Unique<Scene> GLTFLoader::parseScene(int sceneIndex)
	{
		auto scene = createUnique<Scene>("Scene");
		

		
		return scene;
	}

	TransformComponent GLTFLoader::parseTransform(
		const tinygltf::Node& node)
	{
		TransformComponent transform;

		// check translation data
		if (!node.translation.empty())
		{
			transform.setTranslation({
				node.translation[0],
				node.translation[1],
				node.translation[2]
				});
		}

		// check scale data
		if (!node.scale.empty())
		{
			transform.setScale({
				node.scale[0],
				node.scale[1],
				node.scale[2]
				});
		}

		// check rotation data
		if (!node.rotation.empty())
		{
			transform.setRotation(glm::quat
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