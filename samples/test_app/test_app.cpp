#include "test_app.h"

#include "core/entrypoint.h"

TestApp::TestApp() {}

TestApp::~TestApp() { APP_INFO("Destroying Test Application"); }

dusk::Application* dusk::CreateApplication(int argc, char** argv) {
	APP_INFO("Creating Test Application");
	return new TestApp();
}