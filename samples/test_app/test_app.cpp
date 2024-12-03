#include "test_app.h"

#include "core/entrypoint.h"
#include "scene/scene.h"
#include "events/key_event.h"
#include "events/mouse_event.h"

TestApp::TestApp() { }

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

void TestApp::shutdown() { }

void TestApp::onUpdate(TimeStep dt) { }

void TestApp::onEvent(Event& ev)
{
    EventDispatcher dispatcher(ev);

    /*dispatcher.dispatch<KeyPressedEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<KeyReleasedEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<KeyRepeatEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<MouseButtonReleasedEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<MouseScrolledEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });

    dispatcher.dispatch<MouseMovedEvent>(
        [this](Event& ev)
        {
            APP_DEBUG(ev.toString());
            return true;
        });*/
}
