#include "test_lights.h"

#include "core/entrypoint.h"
#include "scene/components/transform.h"
#include "scene/components/camera.h"

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

    Engine::get().loadScene(m_testScene.get());
    Engine::get().setSkyboxVisibility(false);

    auto& cameraTransform       = m_testScene->getMainCameraTransform();
    auto& camera                = m_testScene->getMainCamera();

    cameraTransform.translation = { 0.f, 1.5f, 20.f };
    camera.setViewTarget(cameraTransform.translation, glm::vec3 { 0.f }, {0.f, 1.f, 0.f});

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
