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

    Unique<GameObject> parseNode(
        const tinygltf::Node& node, EntityRegistry& registry);

private:
    tinygltf::Model m_model;
};
} // namespace dusk