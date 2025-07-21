#include "test_scene.h"

#include "core/entrypoint.h"
#include "loaders/stb_image_loader.h"
#include "scene/components/lights.h"

#include <spdlog/fmt/bundled/printf.h>

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
    // std::string scenePath = "assets/scenes/Scene.gltf";

    std::string scenePath
        = "assets/scenes/tea_cup/DiffuseTransmissionTeacup.gltf";
    m_testScene = Scene::createSceneFromGLTF(scenePath);

    // adding ambient light
    auto ambientLight = dusk::createUnique<GameObject>();
    ambientLight->setName("ambient_light");
    auto& aLight = ambientLight->addComponent<AmbientLightComponent>();
    aLight.color = glm::vec4(1.f, 1.f, 1.f, 0.8);
    m_testScene->addGameObject(std::move(ambientLight), m_testScene->getRootId());

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
