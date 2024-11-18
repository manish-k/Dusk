#pragma once

#include "dusk.h"

class TestApp final : public dusk::Application
{
public:
	TestApp();
	~TestApp();

	void onStart() override;
};
