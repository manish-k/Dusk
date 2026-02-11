#pragma once

#include "dusk.h"

#include "core/dtime.h"
#include "backend/vulkan/vk.h"

namespace dusk
{
static constexpr uint32_t MAX_FRAMES_HISTORY    = 100;
static constexpr uint32_t MAX_QUERIES_PER_FRAME = 2 + MAX_RENDER_GRAPH_PASSES * 2; // 2 queries for frame begin/end + 2 queries per pass (begin/end)

struct PassStats
{
    std::string passName  = "";
    TimeStepNs  gpuTimeNs = {};
};

struct FrameStats
{
    TimeStepNs              cpuFrameTime             = {};
    TimeStepNs              gpuFrameTime             = {};

    DynamicArray<PassStats> passStats                = {}; // preserving execution order

    uint64_t                verticesCount            = 0;
    uint64_t                totalVramUsedBytes       = 0;
    uint64_t                totalVramBudgetBytes     = 0;
    uint64_t                totalVramBufferUsedBytes = 0;
    uint64_t                totalVramImageUsedBytes  = 0;
};

class StatsRecorder
{
public:
    StatsRecorder(VulkanContext ctx);
    ~StatsRecorder() = default;

    void init(uint32_t maxFramesInFlightCount);
    void cleanup();
    void retrieveQueryStats();

    void beginFrame(VkCommandBuffer cmdBuffer);
    void endFrame(VkCommandBuffer cmdBuffer);
    void beginPass(VkCommandBuffer cmdBuffer, const std::string& passName);
    void endPass(VkCommandBuffer cmdBuffer);

    void recordCpuFrameTime(TimeStepNs cpuFrameTime);
    void recordGpuMemoryUsage(const VulkanGPUAllocator& gpuAllocator);

private:
    Array<FrameStats, MAX_FRAMES_HISTORY> m_frameStatsHistory      = { {} };

    uint32_t                              m_frameCounter           = 0;
    uint32_t                              m_maxFramesInFlightCount = 0;

    VkDevice                              m_device                 = VK_NULL_HANDLE;
    VulkanGPUAllocator                    m_gpuAllocator           = {};
    VkQueryPool                           m_queryPool              = VK_NULL_HANDLE;
};
} // namespace dusk