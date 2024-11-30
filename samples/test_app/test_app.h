#pragma once

#include "dusk.h"
#include "core/application.h"

class TestApp final : public dusk::Application
{
public:
    TestApp();
    ~TestApp();

    bool start() override;
    void shutdown() override;
    void onUpdate(float dt) override;
    void onEvent(dusk::Event& ev) override;
};
