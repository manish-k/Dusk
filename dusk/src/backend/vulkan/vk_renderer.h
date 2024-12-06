#pragma once

#include "renderer/renderer.h"

#define VOLK_IMPLEMENTATION
#include <volk.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

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