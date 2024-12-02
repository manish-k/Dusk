#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "gltf_loader.h"

namespace dusk
{
GLTFLoader::GLTFLoader()
{
}

Unique<Scene> GLTFLoader::readScene(std::string_view fileName)
{
    tinygltf::TinyGLTF loader;
    std::string        loadErr;
    std::string        loadWarn;
    bool               loadResult;

    loadResult = loader.LoadASCIIFromFile(
        &m_model, &loadErr, &loadWarn, fileName.data());

    if (!loadResult)
    {
        DUSK_ERROR("Error in reading scene. {}", loadErr);
        return createUnique<Scene>("Scene");
    }

    if (!loadWarn.empty())
    {
        DUSK_WARN("{}", loadWarn);
    }

    int sceneIndex = 0;
    if (m_model.scenes.size() == 0)
    {
        DUSK_ERROR("No scenes present in the file");
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
    auto  scene     = createUnique<Scene>("Scene");
    auto& gltfScene = m_model.scenes[sceneIndex];

    // traverse all nodes and attach to the scene
    for (int nodeIndex : gltfScene.nodes)
    {
        traverseSceneNodes(*scene, nodeIndex, scene->getRootId());
    }

    return scene;
}

void GLTFLoader::traverseSceneNodes(
    Scene& scene, int nodeIndex, EntityId parentId)
{
    auto& gltfNode     = m_model.nodes[nodeIndex];
    auto  gameObject   = parseNode(gltfNode, scene.getRegistry());
    auto  gameObjectId = gameObject->getId();

    gameObject->setName(gltfNode.name);

    // parse mesh

    // attach object to the scene
    scene.addGameObject(std::move(gameObject), parentId);

    // traverse children
    for (int childIndex : gltfNode.children)
    {
        traverseSceneNodes(scene, childIndex, gameObjectId);
    }
}

Unique<GameObject> GLTFLoader::parseNode(
    const tinygltf::Node& node, EntityRegistry& registry)
{
    auto  gameObject = createUnique<GameObject>(registry);
    auto& transform  = gameObject->getComponent<TransformComponent>();

    // check translation data
    if (!node.translation.empty())
    {
        transform.setTranslation({ node.translation[0],
                                   node.translation[1],
                                   node.translation[2] });
    }

    // check scale data
    if (!node.scale.empty())
    {
        transform.setScale({ node.scale[0],
                             node.scale[1],
                             node.scale[2] });
    }

    // check rotation data
    if (!node.rotation.empty())
    {
        transform.setRotation(glm::quat {
            (float)node.rotation[0],
            (float)node.rotation[1],
            (float)node.rotation[2],
            (float)node.rotation[3] });
    }

    return gameObject;
}
} // namespace dusk