#include "test_lights.h"

#include "core/entrypoint.h"
#include "scene/components/transform.h"
#include "scene/components/camera.h"
#include "scene/components/lights.h"

TestLights::TestLights()
{
}

TestLights::~TestLights()
{
    APP_INFO("Destroying test scene application");
}

Unique<dusk::Application> dusk::createApplication(int argc, char** argv)
{
    APP_INFO("Creating Test scene application");
    return createUnique<TestLights>();
}

bool TestLights::start()
{
    std::string scenePath = "assets/scenes/test_lights.gltf";

    m_testScene           = Scene::createSceneFromGLTF(scenePath);

    // adding ambient light
    auto ambientLight = dusk::createUnique<GameObject>();
    ambientLight->setName("ambient_light");
    auto& aLight = ambientLight->addComponent<AmbientLightComponent>();
    aLight.color = glm::vec4(1.f, 1.f, 1.f, 0.1);
    m_testScene->addGameObject(std::move(ambientLight), m_testScene->getRootId());

    // adding directional light
    auto directionalLight = dusk::createUnique<GameObject>();
    directionalLight->setName("directional_light_0");
    auto& dLight     = directionalLight->addComponent<DirectionalLightComponent>();
    dLight.direction = glm::vec3(4.f, -2.f, -6.f);
    dLight.color     = glm::vec4(1.f, 1.f, 1.f, 0.2);
    m_testScene->addGameObject(std::move(directionalLight), m_testScene->getRootId());

    // adding point light
    auto pointLight = dusk::createUnique<GameObject>();
    pointLight->setName("point_light_0");
    auto& pointTransform       = pointLight->getComponent<TransformComponent>();
    pointTransform.translation = glm::vec3(0.f, 3.f, 0.f);
    auto& pLight               = pointLight->addComponent<PointLightComponent>();
    pLight.color               = glm::vec4(1.f, 1.f, 1.f, 0.6);
    m_testScene->addGameObject(std::move(pointLight), m_testScene->getRootId());

    // adding spot light
    auto spotLight = dusk::createUnique<GameObject>();
    spotLight->setName("spot_light_0");
    auto& spotTransform       = spotLight->getComponent<TransformComponent>();
    spotTransform.translation = glm::vec3(3.f, 1.5f, 0.f);
    auto& sLight              = spotLight->addComponent<SpotLightComponent>();
    sLight.color              = glm::vec4(1.f, 1.f, 1.f, 0.8);
    sLight.direction          = glm::vec3(0.f, -2.f, 0.f);
    sLight.innerCutOff        = 0.86f; // 30 degrees
    sLight.outerCutOff        = 0.8f;  // ~35 degrees
    m_testScene->addGameObject(std::move(spotLight), m_testScene->getRootId());

    Engine::get().loadScene(m_testScene.get());
    Engine::get().setSkyboxVisibility(false);

    auto& cameraTransform       = m_testScene->getMainCameraTransform();
    auto& camera                = m_testScene->getMainCamera();

    cameraTransform.translation = { 3.f, 2.3f, 10.f };
    camera.setViewTarget(cameraTransform.translation, glm::vec3 { 0.f }, { 0.f, 1.f, 0.f });

    return true;
}

void TestLights::shutdown()
{
    m_testScene = nullptr;
}

void TestLights::onUpdate(TimeStep dt)
{
}

void TestLights::onEvent(dusk::Event& ev)
{
}
