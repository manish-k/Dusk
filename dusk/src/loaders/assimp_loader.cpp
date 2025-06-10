#include "assimp_loader.h"

#include "scene/scene.h"
#include "scene/components/transform.h"
#include "scene/components/mesh.h"

#include "renderer/texture.h"
#include "renderer/material.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <assimp/pbrmaterial.h>

namespace dusk
{

AssimpLoader::AssimpLoader()
{
}

AssimpLoader::~AssimpLoader()
{
}

Unique<Scene> AssimpLoader::readScene(const std::filesystem::path& filePath)
{
    m_sceneDir                 = filePath.parent_path();

    const aiScene* assimpScene = m_importer.ReadFile(filePath.string(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs);

    if (assimpScene == nullptr)
    {
        DUSK_ERROR("Unable to read scene file. {}", m_importer.GetErrorString());
        m_sceneDir = "";
        return nullptr;
    }

    return parseScene(assimpScene);
}

Unique<Scene> AssimpLoader::parseScene(const aiScene* assimpScene)
{
    auto newScene = createUnique<Scene>(assimpScene->mName.C_Str());

    if (assimpScene->mNumMeshes > 0)
    {
        newScene->initMeshesCache(assimpScene->mNumMeshes);
        parseMeshes(*newScene, assimpScene);
    }

    if (assimpScene->HasMaterials())
    {
        newScene->initMaterialCache(assimpScene->mNumMaterials);
        parseMaterials(*newScene, assimpScene);
    }

    if (assimpScene->mRootNode)
    {
        traverseSceneNodes(*newScene, assimpScene->mRootNode, assimpScene, newScene->getRootId());
    }

    return newScene;
}

void AssimpLoader::traverseSceneNodes(Scene& scene, const aiNode* node, const aiScene* aiScene, EntityId parentId)
{
    auto gameObject   = createUnique<GameObject>(parseAssimpNode(node));
    auto gameObjectId = gameObject->getId();

    gameObject->setName(node->mName.C_Str());

    if (node->mNumMeshes > 0)
    {
        auto& meshComponent = gameObject->addComponent<MeshComponent>();

        for (uint32_t index = 0u; index < node->mNumMeshes; ++index)
        {
            uint32_t sceneMeshIndex = node->mMeshes[index];
            meshComponent.meshes.push_back(sceneMeshIndex);
            meshComponent.materials.push_back(aiScene->mMeshes[sceneMeshIndex]->mMaterialIndex);
        }
    }

    // attach object to the scene
    scene.addGameObject(std::move(gameObject), parentId);

    // traverse children
    if (node->mNumChildren > 0)
    {
        for (uint32_t childIndex = 0u; childIndex < node->mNumChildren; ++childIndex)
        {
            traverseSceneNodes(scene, node->mChildren[childIndex], aiScene, gameObjectId);
        }
    }
}

GameObject AssimpLoader::parseAssimpNode(const aiNode* node)
{
    GameObject newGameObject;
    auto&      transform = newGameObject.getComponent<TransformComponent>();

    // get transform from assimp node
    glm::mat4 transformation { 1.0f };

    transformation[0][0] = node->mTransformation.a1;
    transformation[0][1] = node->mTransformation.b1;
    transformation[0][2] = node->mTransformation.c1;
    transformation[0][3] = node->mTransformation.d1;

    transformation[1][0] = node->mTransformation.a2;
    transformation[1][1] = node->mTransformation.b2;
    transformation[1][2] = node->mTransformation.c2;
    transformation[1][3] = node->mTransformation.d2;

    transformation[2][0] = node->mTransformation.a3;
    transformation[2][1] = node->mTransformation.b3;
    transformation[2][2] = node->mTransformation.c3;
    transformation[2][3] = node->mTransformation.d3;

    transformation[3][0] = node->mTransformation.a4;
    transformation[3][1] = node->mTransformation.b4;
    transformation[3][2] = node->mTransformation.c4;
    transformation[3][3] = node->mTransformation.d4;

    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(transformation, scale, rotation, translation, skew, perspective);

    transform.setTranslation(translation);
    transform.setScale(scale);
    transform.setRotation(rotation);

    return newGameObject;
}

void AssimpLoader::parseMeshes(Scene& scene, const aiScene* aiScene)
{
    for (uint32_t meshIndex = 0u; meshIndex < aiScene->mNumMeshes; ++meshIndex)
    {
        DynamicArray<Vertex>   vertices {};
        DynamicArray<uint32_t> indices {};

        aiMesh*                mesh = aiScene->mMeshes[meshIndex];

        vertices.reserve(mesh->mNumVertices);
        indices.reserve(mesh->mNumFaces * 3); // Assumption, face is triangular

        for (uint32_t vertexIndex = 0u; vertexIndex < mesh->mNumVertices; ++vertexIndex)
        {
            Vertex v;

            v.position = glm::vec3(mesh->mVertices[vertexIndex].x, mesh->mVertices[vertexIndex].y, mesh->mVertices[vertexIndex].z);

            if (mesh->HasNormals())
            {
                v.normal = glm::vec3(mesh->mNormals[vertexIndex].x, mesh->mNormals[vertexIndex].y, mesh->mNormals[vertexIndex].z);
            }

            if (mesh->HasTextureCoords(0))
            {
                v.uv = glm::vec2(mesh->mTextureCoords[0][vertexIndex].x, 1.f - mesh->mTextureCoords[0][vertexIndex].y);
            }

            vertices.push_back(v);
        }

        for (uint32_t faceIndex = 0u; faceIndex < mesh->mNumFaces; ++faceIndex)
        {
            DASSERT(mesh->mFaces[faceIndex].mNumIndices == 3, "Face is not triangular");
            for (uint32_t vertexIndex = 0u; vertexIndex < 3; ++vertexIndex)
                indices.push_back(mesh->mFaces[faceIndex].mIndices[vertexIndex]);
        }

        scene.initSubMesh(meshIndex, vertices, indices);
    }
}

std::string AssimpLoader::getTexturePath(aiMaterial* mat, aiTextureType type)
{
    aiString path;
    aiReturn result = mat->GetTexture(type, 0, &path);

    if (result == aiReturn_FAILURE)
        return "";

    return std::string(path.C_Str());
}

void AssimpLoader::parseMaterials(Scene& scene, const aiScene* aiScene)
{
    for (uint32_t matIndex = 0; matIndex < aiScene->mNumMaterials; ++matIndex)
    {
        aiMaterial* aiMat = aiScene->mMaterials[matIndex];

        // load diffuse texture
        std::string baseColorTexturePath = getTexturePath(aiMat, aiTextureType_BASE_COLOR);

        uint32_t    diffuseTexId         = -1;
        if (!baseColorTexturePath.empty())
        {
            auto texturePath = (m_sceneDir / baseColorTexturePath).string();
            diffuseTexId     = scene.loadTexture(texturePath);
        }
        else
        {
            diffuseTexId = scene.getDefaultTextureId();
        }

        aiColor3D diffuseColor { 1.0f };
        aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);

        aiColor3D pbrBaseColor { 1.0f };
        aiMat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, pbrBaseColor);

        float alpha = 1.f;
        aiMat->Get(AI_MATKEY_OPACITY, alpha);

        Material mat;

        mat.albedoColor = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, alpha);

        mat.albedoTexId = diffuseTexId;

        scene.addMaterial(mat);
    }
}

} // namespace dusk