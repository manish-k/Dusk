#pragma once

#include "dusk.h"
#include "core/application.h"
#include "scene/scene.h"
#include "renderer/texture.h"

using namespace dusk;

class TestScene : public Application
{
public:
    TestScene();
    ~TestScene();

    bool start() override;
    void shutdown() override;
    void onUpdate(TimeStep dt) override;
    void onEvent(dusk::Event& ev) override;

private:
    Unique<Scene>   m_testScene;
    Unique<Texture> m_testTexture;
};