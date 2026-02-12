#pragma once

#include "dusk.h"

#include "core/dtime.h"
#include "backend/vulkan/vk.h"
#include "renderer/render_graph.h"

namespace dusk
{
static constexpr uint32_t MAX_FRAMES_HISTORY    = 100;
static constexpr uint32_t MAX_QUERIES_PER_FRAME = 2 + MAX_RENDER_GRAPH_PASSES * 2; // 2 queries for frame begin/end + 2 queries per pass (begin/end)
static constexpr float    EMA_ALPHA             = 0.05f;                           // Smoothing factor for Exponential Moving Average

struct PassStats
{
    std::string passName  = "";
    TimeStepNs  gpuTimeNs = {};
};

struct FrameStats
{
    TimeStepNs              cpuFrameTime             = {};
    TimeStepNs              gpuFrameTime             = {};

    uint64_t                verticesCount            = 0LLU;
    uint64_t                totalVramUsedBytes       = 0LLU;
    uint64_t                totalVramBudgetBytes     = 0LLU;
    uint64_t                totalVram                = 0LLU;
    uint64_t                totalVramBufferUsedBytes = 0LLU;
    uint64_t                totalVramImageUsedBytes  = 0LLU;

    DynamicArray<PassStats> passStats                = {}; // preserving execution order
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
    StatsRecorder();
    ~StatsRecorder() = default;

    /**
     * @brief Initialize the StatsRecorder
     * @param total frames in flight
     * @return true if initialization is successful, false otherwise
     */
    bool init(uint32_t maxFramesInFlightCount);

    /**
     * @brief cleanup vulkan resources
     */
    void cleanup();

    /**
     * @brief Retrieve query results from the GPU for N - total frames in flight
     */
    void retrieveQueryStats();

    /**
     * @brief Begins collection of current frame stats. All record commands should be called after this.
     * @param command buffer for the current frame
     */
    void beginFrame(VkCommandBuffer cmdBuffer);

    /**
     * @brief Ends collection of current frame stats
     * @param command buffer for the current frame
     */
    void endFrame(VkCommandBuffer cmdBuffer);

    /**
     * @brief Begins collection of pass stats in render graph execution
     * @param command buffer for the current frame
     * @param name of the pass
     * @param index of pass in execution order of the render graph
     */
    void beginPass(
        VkCommandBuffer    cmdBuffer,
        const std::string& passName,
        uint32_t           passOrderedIndex);

    /**
     * @brief Ends collection of pass stats in render graph execution
     * @param command buffer for the current frame
     * @param index of pass in execution order of the render graph
     */
    void endPass(VkCommandBuffer cmdBuffer, uint32_t passOrderedIndex);

    /**
     * @brief Record CPU frame time for the current frame
     * @param frame time in nanoseconds for the current frame
     */
    void recordCpuFrameTime(TimeStepNs cpuFrameTime);

    /**
     * @brief Record GPU memory usage for the current session
     * @param GPU allocator pointer
     */
    void recordGpuMemoryUsage(const VulkanGPUAllocator* gpuAllocator);

    /**
     * @brief Get aggregate stats based on the defined window size
     * @return AggregateStats structure containing average, max, and EMA of CPU and GPU frame times
     */
    AggregateStats getAggregateStats() const { return m_aggregateStats; }

    /**
     * @brief Returns the FrameStats for the frame that occurred three frames ago
     * @return A FrameStats object containing stats of the frame
     */
    FrameStats getThirdLastFrameStats() const;

public:
    /**
     * @brief Get static instance of StatsRecorder
     * @return Pointer to the StatsRecorder instance
     */
    static StatsRecorder* get() { return s_instance; };

private:
    Array<FrameStats, MAX_FRAMES_HISTORY> m_frameStatsHistory      = { {} };

    uint32_t                              m_frameCounter           = 0;
    uint32_t                              m_maxFramesInFlightCount = 0;

    VkDevice                              m_device                 = VK_NULL_HANDLE;
    VkPhysicalDevice                      m_physicalDevice         = VK_NULL_HANDLE;
    VkQueryPool                           m_queryPool              = VK_NULL_HANDLE;

    // Aggregate stats based on history buffer
    AggregateStats m_aggregateStats = {};

private:
    static StatsRecorder* s_instance;
};
} // namespace dusk