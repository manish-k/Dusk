#include "stats_recorder.h"

#include "renderer/render_graph.h"

#include "backend/vulkan/vk_device.h"

namespace dusk
{
StatsRecorder::StatsRecorder(VulkanContext ctx) :
    m_device(ctx.device),
    m_gpuAllocator(ctx.gpuAllocator)
{
}

void StatsRecorder::init(uint32_t maxFramesInFlightCount)
{
    m_frameStatsHistory.fill({});

    for (uint32_t i = 0; i < maxFramesInFlightCount; ++i)
    {
        VkQueryPoolCreateInfo queryPoolCreateInfo { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
        queryPoolCreateInfo.queryType  = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = MAX_QUERIES_PER_FRAME;
        queryPoolCreateInfo.flags      = 0;

        VkDevice     device            = VkGfxDevice::getSharedVulkanContext().device;
        VulkanResult result            = vkCreateQueryPool(device, &queryPoolCreateInfo, nullptr, &m_queryPool);

        if (result.hasError())
        {
            DUSK_ERROR("Failed to create query pool for stats recorder: {}", result.toString());
        }

#ifdef VK_RENDERER_DEBUG
        vkdebug::setObjectName(
            device,
            VK_OBJECT_TYPE_QUERY_POOL,
            (uint64_t)m_queryPool,
            "stats_recorder_timestamp_query_pool");
#endif // VK_RENDERER_DEBUG
    }
}

void StatsRecorder::cleanup()
{
    VkDevice device = VkGfxDevice::getSharedVulkanContext().device;
    vkDestroyQueryPool(device, m_queryPool, nullptr);
}

void StatsRecorder::retrieveQueryStats()
{
    uint64_t queryResults[MAX_QUERIES_PER_FRAME] = {};

    if (m_frameCounter >= m_maxFramesInFlightCount)
    {
        VkDevice device          = VkGfxDevice::getSharedVulkanContext().device;

        uint32_t frameToRetrieve = (m_frameCounter - m_maxFramesInFlightCount);
        uint32_t queryIndex      = (frameToRetrieve % m_maxFramesInFlightCount) * MAX_QUERIES_PER_FRAME;

        // Retrieve query results for the frame that has completed GPU execution
        VulkanResult result = vkGetQueryPoolResults(
            device,
            m_queryPool,
            queryIndex,            // start index of the queries for the current frame
            MAX_QUERIES_PER_FRAME, // count of queries for the frame
            sizeof(queryResults),  // size of the results buffer
            queryResults,          // buffer to store results
            sizeof(uint64_t),      // stride between results
            VK_QUERY_RESULT_64_BIT);

        if (result.hasError())
        {
            DUSK_ERROR("Failed to retrieve query results for stats recorder at frame {}: {}", m_frameCounter, result.toString());
            return;
        }

        uint32_t ringBufferIndex = frameToRetrieve % MAX_FRAMES_HISTORY;

        // GPU Frame time
        uint64_t gpuFrameTimeNs                           = queryResults[1] - queryResults[0];
        m_frameStatsHistory[ringBufferIndex].gpuFrameTime = TimeStepNs(gpuFrameTimeNs);

        // Pass times
        uint32_t totalPasses = m_frameStatsHistory[ringBufferIndex].passStats.size();
        for (uint32_t index; index < totalPasses; ++index)
        {
            auto&    passStats      = m_frameStatsHistory[ringBufferIndex].passStats[index];
            uint32_t passQueryIndex = 2 + index * 2;
            uint64_t passTimeNs     = queryResults[passQueryIndex + 1] - queryResults[passQueryIndex];

            passStats.gpuTimeNs     = TimeStepNs(passTimeNs);
        }
    }
}

void StatsRecorder::beginFrame(VkCommandBuffer cmdBuffer)
{
    uint32_t queryIndex = (m_frameCounter % m_maxFramesInFlightCount) * MAX_QUERIES_PER_FRAME;

    vkCmdResetQueryPool(
        cmdBuffer,
        m_queryPool,
        queryIndex,             // start index of the queries for the current frame
        MAX_QUERIES_PER_FRAME); // count of queries for the frame

    vkCmdWriteTimestamp(
        cmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        m_queryPool,
        queryIndex); // write gpu timestamp for frame begin at the first index
}

void StatsRecorder::endFrame(VkCommandBuffer cmdBuffer)
{
    uint32_t queryIndex = (m_frameCounter % m_maxFramesInFlightCount) * MAX_QUERIES_PER_FRAME;

    vkCmdWriteTimestamp(
        cmdBuffer,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        m_queryPool,
        queryIndex + 1); // write gpu timestamp for frame end at the second index

    m_frameCounter++;
}

void StatsRecorder::beginPass(VkCommandBuffer cmdBuffer, const std::string& passName)
{
    uint32_t passCount  = m_frameStatsHistory[m_frameCounter % MAX_FRAMES_HISTORY].passStats.size();
    uint32_t queryIndex = (m_frameCounter % m_maxFramesInFlightCount) * MAX_QUERIES_PER_FRAME + 2 + passCount * 2;

    vkCmdWriteTimestamp(
        cmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        m_queryPool,
        queryIndex); // write gpu timestamp for pass begin

    m_frameStatsHistory[m_frameCounter % MAX_FRAMES_HISTORY].passStats.emplace_back(passName, TimeStepNs(0));
}

void StatsRecorder::endPass(VkCommandBuffer cmdBuffer)
{
    uint32_t passCount  = m_frameStatsHistory[m_frameCounter % MAX_FRAMES_HISTORY].passStats.size();
    uint32_t queryIndex = (m_frameCounter % m_maxFramesInFlightCount) * MAX_QUERIES_PER_FRAME + 2 + (passCount - 1) * 2 + 1;

    vkCmdWriteTimestamp(
        cmdBuffer,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        m_queryPool,
        queryIndex); // write gpu timestamp for pass end
}

void StatsRecorder::recordCpuFrameTime(TimeStepNs cpuFrameTime)
{
    m_frameStatsHistory[m_frameCounter % MAX_FRAMES_HISTORY].cpuFrameTime = cpuFrameTime;
}

void StatsRecorder::recordGpuMemoryUsage(const VulkanGPUAllocator& gpuAllocator)
{
    auto&     frameStats = m_frameStatsHistory[m_frameCounter % MAX_FRAMES_HISTORY];

    VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
    vmaGetHeapBudgets(gpuAllocator.vmaAllocator, budgets);

    uint64_t totalHeapBudgetBytes = 0;
    uint64_t totalHeapUsageBytes  = 0;

    for (uint32_t heapIndex = 0; heapIndex < VK_MAX_MEMORY_HEAPS; ++heapIndex)
    {
        totalHeapBudgetBytes += budgets[heapIndex].budget;
        totalHeapUsageBytes += budgets[heapIndex].usage;
    }

    frameStats.totalVramUsedBytes       = totalHeapUsageBytes;
    frameStats.totalVramBudgetBytes     = totalHeapBudgetBytes;
    frameStats.totalVramBufferUsedBytes = gpuAllocator.allocatedBufferBytes;
    frameStats.totalVramImageUsedBytes  = gpuAllocator.allocatedImageBytes;

} // namespace dusk