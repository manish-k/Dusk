#pragma once

#include "dusk.h"

#include "core/dtime.h"
#include "backend/vulkan/vk.h"
#include "renderer/render_graph.h"

namespace dusk
{
static constexpr uint32_t MAX_FRAMES_HISTORY    = 100;
static constexpr uint32_t MAX_QUERIES_PER_FRAME = 2 + MAX_RENDER_GRAPH_PASSES * 2; // 2 queries for frame begin/end + 2 queries per pass (begin/end)
static constexpr float    EMA_ALPHA             = 0.2f;                            // Smoothing factor for Exponential Moving Average

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

struct AggregateStats
{
    TimeStepNs rollingSumCpuTimeNs = {};
    TimeStepNs avgCpuTimeNs        = {};
    TimeStepNs maxCpuTimeNs        = {};
    TimeStepNs emaCpuTimeNs        = {};
    TimeStepNs rollingSumGpuTimeNs = {};
    TimeStepNs avgGpuTimeNs        = {};
    TimeStepNs maxGpuTimeNs        = {};
    TimeStepNs emaGpuTimeNs        = {};
};

class StatsRecorder
{
public:
    StatsRecorder(VulkanContext ctx);
    ~StatsRecorder() = default;

    bool init(uint32_t maxFramesInFlightCount);
    void cleanup();
    void retrieveQueryStats();

    void beginFrame(VkCommandBuffer cmdBuffer);
    void endFrame(VkCommandBuffer cmdBuffer);
    void beginPass(
        VkCommandBuffer    cmdBuffer,
        const std::string& passName,
        uint32_t           passOrderedIndex);
    void           endPass(VkCommandBuffer cmdBuffer, uint32_t passOrderedIndex);

    void           recordCpuFrameTime(TimeStepNs cpuFrameTime);
    void           recordGpuMemoryUsage(const VulkanGPUAllocator& gpuAllocator);

    AggregateStats getAggregateStats() const { return m_aggregateStats; }
    FrameStats     getThirdLastFrameStats() const { return m_frameStatsHistory[(m_frameCounter + MAX_FRAMES_HISTORY - 3) % MAX_FRAMES_HISTORY]; }

public:
    static StatsRecorder* get() { return s_instance; };

private:
    Array<FrameStats, MAX_FRAMES_HISTORY> m_frameStatsHistory      = { {} };

    uint32_t                              m_frameCounter           = 0;
    uint32_t                              m_maxFramesInFlightCount = 0;

    VkDevice                              m_device                 = VK_NULL_HANDLE;
    VulkanGPUAllocator                    m_gpuAllocator           = {};
    VkQueryPool                           m_queryPool              = VK_NULL_HANDLE;

    AggregateStats                        m_aggregateStats         = {};

private:
    static StatsRecorder* s_instance;
};
} // namespace dusk