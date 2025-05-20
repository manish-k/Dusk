#pragma once

#include "dusk.h"
#include "vk.h"

#include <string>

namespace dusk
{
struct RenderGraphNode
{
    std::string name;
};

class RenderGraph
{
public:
    void addPass();
    void executeContext();
    void setPassContext();

private:
    DynamicArray<RenderGraphNode> m_passes;
};
} // namespace dusk