#include "test_sponza.h"

#include "core/entrypoint.h"
#include "scene/components/lights.h"
#include "scene/camera_controller.h"

TestSponza::TestSponza()
{
}

TestSponza::~TestSponza()
{
    APP_INFO("Destroying test scene application");
}

Unique<dusk::Application> dusk::createApplication(int argc, char** argv)
{
    APP_INFO("Creating Test sponza application");
    return createUnique<TestSponza>();
}

bool TestSponza::start()
{
    std::string scenePath = "assets/scenes/Sponza.gltf";

    m_testSponza          = Scene::createSceneFromGLTF(scenePath);

    // adding directional light
    auto directionalLight = dusk::createUnique<GameObject>();
    directionalLight->setName("directional_light_0");
    auto& dLight     = directionalLight->addComponent<DirectionalLightComponent>();
    dLight.direction = glm::vec3(0.735f, -5.5f, 0.9f);
    dLight.color     = glm::vec4(1.f, 1.f, 1.f, 0.9);
    m_testSponza->addGameObject(std::move(directionalLight), m_testSponza->getRootId());

    if (m_testSponza)
        Engine::get().loadScene(m_testSponza.get());

    auto& cameraController = m_testSponza.get()->getMainCameraController();
    cameraController.setPosition({ 9.6f, 1.f, -0.9f });
    cameraController.setViewDirection({ -0.9f, 0.1f, 0.4f });

    return true;
}

void TestSponza::shutdown()
{
    m_testSponza = nullptr;
}

void TestSponza::onUpdate(TimeStep dt)
{
}

void TestSponza::onEvent(dusk::Event& ev)
{
}
