#include "dusk.h"

#include "vk.h"
#include "vk_types.h"

namespace dusk
{
namespace vulkan
{
inline VulkanResult createCmdBufferPool(VkDevice device, uint32_t queueFamilyIndex, uint32_t numBuffers, VulkanCmdBufferPool* pOutPool)
{
    pOutPool->device           = device;
    pOutPool->queueFamilyIndex = queueFamilyIndex;

    pOutPool->commandBuffers.reserve(numBuffers);

    VkCommandPoolCreateInfo poolInfo { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VulkanResult result       = vkCreateCommandPool(device, &poolInfo, nullptr, &pOutPool->commandPool);
    if (result.hasError())
    {
        DUSK_ERROR("Failed to create command pool: {}", result.toString());
        return result;
    }

    pOutPool->commandBuffers.resize(numBuffers);

    VkCommandBufferAllocateInfo allocInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool        = pOutPool->commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = numBuffers;

    result                       = vkAllocateCommandBuffers(device, &allocInfo, pOutPool->commandBuffers.data());

    if (result.hasError())
    {
        DUSK_ERROR("Failed to allocate command buffers: {}", result.toString());
    }

    return result;
}

inline void resetCmdBufferPool(VkDevice device, VulkanCmdBufferPool* pool)
{
    vkResetCommandPool(device, pool->commandPool, 0);
    pool->nextAvailableIndex = 0u;
}

inline VkCommandBuffer getNextCmdBuffer(VulkanCmdBufferPool* pool)
{
    if (pool->nextAvailableIndex >= pool->commandBuffers.size())
    {
        VulkanResult result = allocateMoreCmdBuffers(pool, pool->commandBuffers.size());
        if (result.hasError())
        {
            return VK_NULL_HANDLE;
        }
    }
    return pool->commandBuffers[pool->nextAvailableIndex++];
}

inline VulkanResult allocateMoreCmdBuffers(VulkanCmdBufferPool* pool, uint32_t numBuffers)
{
    uint32_t currentSize = static_cast<uint32_t>(pool->commandBuffers.size());
    uint32_t newSize     = currentSize + numBuffers;
    pool->commandBuffers.resize(newSize);

    VkCommandBufferAllocateInfo allocInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool        = pool->commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = numBuffers;

    VulkanResult result          = vkAllocateCommandBuffers(pool->device, &allocInfo, pool->commandBuffers.data() + currentSize);
    if (result.hasError())
    {
        DUSK_ERROR("Failed to allocate command buffers: {}", result.toString());
    }
    else
        pool->nextAvailableIndex = currentSize; // reset to the first of the newly allocated buffers

    return result;
}
}; // namespace vulkan

} // namespace dusk