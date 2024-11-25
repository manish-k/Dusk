#pragma once

#include "dusk.h"
#include "core/base.h"
#include "scene/scene.h"

#include <tiny_gltf.h>
#include <string_view>

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

		Unique<Scene> readScene(std::string_view fileName);

	private:
		Unique<Scene> parseScene(int sceneIndex);
		
		void traverseSceneNodes(
			Scene& scene, int nodeIndex, EntityId parentId);
		
		TransformComponent parseTransform(
			const tinygltf::Node& node);

	private:
		tinygltf::Model m_model;
	};
}