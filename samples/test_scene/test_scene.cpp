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
    std::string scenePath = "assets/scenes/Cube.gltf";
    m_testScene           = Scene::createSceneFromGLTF(scenePath);

    Engine::get().loadScene(m_testScene.get());

    std::string     texturePath = "assets/scenes/Cube_BaseColor.png";
    Unique<Image>   image       = ImageLoader::readImage(texturePath);
    
    m_testTexture               = createUnique<Texture>(1);
    m_testTexture->init(*image);
;
    ImageLoader::freeImage(*image);
    
    return true;
}

void TestScene::shutdown()
{
    m_testTexture->free();

    m_testScene = nullptr;
}

void TestScene::onUpdate(TimeStep dt)
{
}

void TestScene::onEvent(dusk::Event& ev)
{
}
