#include "test_app.h"

#include "core/entrypoint.h"
#include "scene/scene.h"

using namespace dusk;

TestApp::TestApp()
{

}

TestApp::~TestApp() { APP_INFO("Destroying Test Application"); }

dusk::Application* dusk::createApplication(int argc, char** argv)
{
	APP_INFO("Creating Test Application");
	return new TestApp();
}

void TestApp::onStart()
{
	Scene testScene{ "Test" };
	testScene.addGameObject(
		createUnique<GameObject>(testScene.getRegistry()),
		testScene.getRootId());
}