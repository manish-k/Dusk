#include "test_sponza.h"

#include "core/entrypoint.h"

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
