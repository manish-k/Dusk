#include "ibl_generator.h"

#include "core/entrypoint.h"
#include "scene/components/lights.h"
#include "scene/components/transform.h"

/*
Pielines required
#1 HDR to cubemap
#2 Irradiance map
#3 Prefiltered map
*/

IBLGenerator::IBLGenerator()
{
}

IBLGenerator::~IBLGenerator()
{
}

Unique<dusk::Application> dusk::createApplication(int argc, char** argv)
{
    return createUnique<IBLGenerator>();
}

bool IBLGenerator::start()
{
    return true;
}

void IBLGenerator::shutdown()
{
}

void IBLGenerator::onUpdate(TimeStep dt)
{
}

void IBLGenerator::onEvent(dusk::Event& ev)
{
}
