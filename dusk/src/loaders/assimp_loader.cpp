#include "assimp_loader.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace dusk
{

AssimpLoader::AssimpLoader()
{
}

AssimpLoader::~AssimpLoader()
{
}

Unique<Scene> AssimpLoader::readScene(std::string& fileName)
{
    const aiScene* assimpScene = m_importer.ReadFile(fileName, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (assimpScene == nullptr)
    {
        DUSK_ERROR("Unable to read scene file. {}", m_importer.GetErrorString());
        return nullptr;
    }

    return createUnique<Scene>(parseScene(assimpScene));
}

Unique<Scene> AssimpLoader::parseScene(const aiScene* scene)
{
    auto newScene = createUnique<Scene>(scene->mName.C_Str());

    return newScene;
}

void AssimpLoader::traverseSceneNodes(Scene& scene, aiNode* node, EntityId parentId)
{
    auto gameObject   = createUnique<GameObject>(parseAssimpNode(node));
    auto gameObjectId = gameObject->getId();

    gameObject->setName(node->mName.C_Str());

    // parse mesh

    // attach object to the scene
    scene.addGameObject(std::move(gameObject), parentId);

    // traverse children
    if (node->mNumChildren > 0)
    {
        for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        {
            traverseSceneNodes(scene, node->mChildren[childIndex], gameObjectId);
        }
    }
}

GameObject AssimpLoader::parseAssimpNode(aiNode* node)
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

    transformation[3][1] = node->mTransformation.a4;
    transformation[3][2] = node->mTransformation.b4;
    transformation[3][3] = node->mTransformation.c4;
    transformation[3][4] = node->mTransformation.d4;

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

} // namespace dusk