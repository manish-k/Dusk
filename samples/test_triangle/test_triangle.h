#pragma once

#include "dusk.h"
#include "core/application.h"

using namespace dusk;

class TestTriangle final : public dusk::Application
{
public:
    TestTriangle();
    ~TestTriangle();

    bool start() override;
    void shutdown() override;
    void onUpdate(TimeStep dt) override;
    void onEvent(dusk::Event& ev) override;
};
