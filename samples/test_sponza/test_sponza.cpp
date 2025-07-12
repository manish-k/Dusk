#include "test_sponza.h"

#include "core/entrypoint.h"
#include "scene/components/lights.h"

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

    // adding ambient light
    auto ambientLight = dusk::createUnique<GameObject>();
    ambientLight->setName("ambient_light");
    auto& aLight = ambientLight->addComponent<AmbientLightComponent>();
    aLight.color = glm::vec4(1.f, 1.f, 1.f, 0.8);
    m_testSponza->addGameObject(std::move(ambientLight), m_testSponza->getRootId());

    if (m_testSponza)
        Engine::get().loadScene(m_testSponza.get());

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
