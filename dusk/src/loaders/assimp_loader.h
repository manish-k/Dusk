#pragma once

#include "dusk.h"
#include "scene/scene.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace dusk
{
class AssimpLoader
{
public:
    AssimpLoader();
    ~AssimpLoader();

    Unique<Scene> readScene(const std::string& fileName);

private:
    Unique<Scene> parseScene(const aiScene* scene);
    void          parseMeshes(Scene& scene, const aiScene* aiScene);

    void          traverseSceneNodes(Scene& scene, const aiNode* node, const aiScene* aiScene, EntityId parentId);

    GameObject    parseAssimpNode(const aiNode* node);

private:
    Assimp::Importer m_importer;
};
} // namespace dusk