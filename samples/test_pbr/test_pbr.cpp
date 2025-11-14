#include "test_pbr.h"

#include "core/entrypoint.h"
#include "scene/components/lights.h"
#include "scene/components/transform.h"

TestPBR::TestPBR()
{
}

TestPBR::~TestPBR()
{
    APP_INFO("Destroying test pbr application");
}

Unique<dusk::Application> dusk::createApplication(int argc, char** argv)
{
    APP_INFO("Creating Test PBR application");
    return createUnique<TestPBR>();
}

bool TestPBR::start()
{
    std::string scenePath = "assets/scenes/metallic-roughness/test_pbr.gltf";

    m_testPBR             = Scene::createSceneFromGLTF(scenePath);

    // adding directional light
    /*auto directionalLight = dusk::createUnique<GameObject>();
    directionalLight->setName("directional_light_0");
    auto& dLight     = directionalLight->addComponent<DirectionalLightComponent>();
    dLight.direction = glm::vec3(-2.f, -2.f, -6.f);
    dLight.color     = glm::vec4(1.f, 1.f, 1.f, 0.8);
    m_testPBR->addGameObject(std::move(directionalLight), m_testPBR->getRootId());*/

    // adding point light
    auto pointLight = dusk::createUnique<GameObject>();
    pointLight->setName("point_light_0");
    auto& pointTransform       = pointLight->getComponent<TransformComponent>();
    pointTransform.translation = glm::vec3(0.f, 3.f, 0.f);
    auto& pLight               = pointLight->addComponent<PointLightComponent>();
    pLight.color               = glm::vec4(1.f, 1.f, 1.f, 0.6);
    m_testPBR->addGameObject(std::move(pointLight), m_testPBR->getRootId());

    if (m_testPBR)
        Engine::get().loadScene(m_testPBR.get());

    return true;
}

void TestPBR::shutdown()
{
    m_testPBR = nullptr;
}

void TestPBR::onUpdate(TimeStep dt)
{
}

void TestPBR::onEvent(dusk::Event& ev)
{
}
