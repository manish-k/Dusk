#pragma once

#include "dusk.h"
#include "core/application.h"
#include "scene/scene.h"

using namespace dusk;

/*
Application to convert hdr images to cubemaps and also generate
irradiance and pre-filtered maps for diffuse and specular ibl.
*/

class IBLGenerator : public Application
{
public:
    IBLGenerator();
    ~IBLGenerator();

    bool start() override;
    void shutdown() override;
    void onUpdate(TimeStep dt) override;
    void onEvent(dusk::Event& ev) override;

private:
};