#pragma once

#include "core/base.h"
#include "scene/scene.h"

#include <tiny_gltf.h>
#include <string>

namespace dusk
{
	/**
	 * @brief 
	 */
	class GLTFLoader
	{
	public:
		GLTFLoader();
		~GLTFLoader() = default;

		Unique<Scene> readScene(std::string_view fileName, int sceneIndex);

	private:
		Scene loadScene(int sceneIndex);

		Unique<GameObject> parseNode(const tinygltf::Node& node);
	};
}