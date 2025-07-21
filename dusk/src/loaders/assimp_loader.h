#pragma once

#include "dusk.h"
#include "scene/entity.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>

namespace dusk
{
class Scene;
class GameObject;

class AssimpLoader
{
public:
    AssimpLoader();
    ~AssimpLoader();

    Unique<Scene> readScene(const std::filesystem::path& filePath);

private:
    Unique<Scene> parseScene(const aiScene* scene);
    void          parseMeshes(Scene& scene, const aiScene* aiScene);
    void          parseMaterials(Scene& scene, const aiScene* aiScene);

    void          traverseSceneNodes(Scene& scene, const aiNode* node, const aiScene* aiScene, EntityId parentId);

    GameObject    parseAssimpNode(const aiNode* node);

    std::filesystem::path getGltfTexturePath(aiMaterial* mat);
    std::filesystem::path getTexturePath(aiMaterial* mat, aiTextureType type);
    std::filesystem::path getGltfMRTexturePath(aiMaterial* mat);
    int32_t               read2DTexture(std::filesystem::path texPath) const;

private:
    Assimp::Importer m_importer;

    std::filesystem::path m_sceneDir = "";

    bool                  m_isGltf   = false;
};
} // namespace dusk