#include "test_app.h"

#include "core/entrypoint.h"
#include "scene/scene.h"

using namespace dusk;

TestApp::TestApp() {}

TestApp::~TestApp() { APP_INFO("Destroying Test Application"); }

Unique<dusk::Application> dusk::createApplication(int argc, char** argv)
{
    APP_INFO("Creating Test Application");
    return createUnique<TestApp>();
}

bool TestApp::start()
{
    auto scene1 = Scene::createSceneFromGLTF("assets/scenes/test_scene.gltf");

    return true;
}

void TestApp::shutdown() {}

void TestApp::onUpdate(float dt) {}

void TestApp::onEvent(dusk::Event& ev) {}
