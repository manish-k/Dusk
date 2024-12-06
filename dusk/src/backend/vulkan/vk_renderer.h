#pragma once

#include "renderer/renderer.h"

#define VOLK_IMPLEMENTATION
#include "volk.h"

namespace dusk
{
	class VkRenderer final : public Renderer
	{
        public:
            VkRenderer();
            ~VkRenderer() override;

			bool init() override;
	};
}