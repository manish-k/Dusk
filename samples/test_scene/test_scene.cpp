#include "test_scene.h"

#include "core/entrypoint.h"
#include "scene/components/lights.h"
#include "scene/camera_controller.h"

TestScene::TestScene()
{
}

TestScene::~TestScene()
{
    APP_INFO("Destroying test scene application");
}

Unique<dusk::Application> dusk::createApplication(int argc, char** argv)
{
    APP_INFO("Creating Test scene application");
    return createUnique<TestScene>();
}

bool TestScene::start()
{
    // std::string scenePath = "assets/scenes/EnvironmentTest.gltf";
    // std::string scenePath = "assets/scenes/2_cubes.gltf";
    // std::string scenePath = "assets/scenes/Scene.gltf";
    std::string scenePath = "D:/resources/scene/bistro_gltf/bistro.gltf";
    // std::string scenePath
    //     = "assets/scenes/tea_cup/DiffuseTransmissionTeacup.gltf";
    m_testScene = Scene::createSceneFromGLTF(scenePath);

    // adding directional light
    auto directionalLight = dusk::createUnique<GameObject>();
    directionalLight->setName("directional_light_0");
    auto& dLight     = directionalLight->addComponent<DirectionalLightComponent>();
    dLight.direction = glm::vec3(3.3f, -4.2f, 1.685f);
    dLight.color     = glm::vec4(1.f, 1.f, 1.f, 5.2);
    m_testScene->addGameObject(std::move(directionalLight), m_testScene->getRootId());

    // adding point light
    /*auto pointLight = dusk::createUnique<GameObject>();
    pointLight->setName("point_light_0");
    pointLight->setPosition(glm::vec3(0.f, 3.f, 0.f));
    auto& pLight               = pointLight->addComponent<PointLightComponent>();
    pLight.color               = glm::vec4(1.f, 1.f, 1.f, 0.6);
    m_testScene->addGameObject(std::move(pointLight), m_testScene->getRootId());*/

     auto& cameraController = m_testScene.get()->getMainCameraController();
     cameraController.setPosition({ -21.58f, 10.19f,-6.78f });
     cameraController.setViewDirection({ -0.83f, 0.364f, -0.148f });

    Engine::get().loadScene(m_testScene.get());

    return true;
}

void TestScene::shutdown()
{
    m_testScene = nullptr;
}

void TestScene::onUpdate(TimeStep dt)
{
}

void TestScene::onEvent(dusk::Event& ev)
{
}
