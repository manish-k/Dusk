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

    Unique<Scene> readScene(std::string& fileName);

private:
    Unique<Scene> parseScene(const aiScene* scene);

    void          traverseSceneNodes(Scene& scene, aiNode* node, EntityId parentId);
    GameObject    parseAssimpNode(aiNode* node);

private:
    Assimp::Importer m_importer;
};
} // namespace dusk