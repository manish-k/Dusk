#pragma once

#include "dusk.h"

namespace dusk
{
	struct MeshComponent
	{
            DynamicArray<uint32_t> meshes {};
            DynamicArray<uint32_t> materials {};
	};
}