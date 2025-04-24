#include "test_scene.h"

#include "core/entrypoint.h"
#include "loaders/stb_image_loader.h"

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
    std::string scenePath = "assets/scenes/Scene.gltf";
    m_testScene           = Scene::createSceneFromGLTF(scenePath);

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
