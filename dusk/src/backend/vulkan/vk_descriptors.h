#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"
#include "vk_device.h"

namespace dusk
{
struct VkGfxDescriptorSetLayout
{
    VkDevice                                        device = VK_NULL_HANDLE;
    VkDescriptorSetLayout                           layout = VK_NULL_HANDLE;
    HashMap<uint32_t, VkDescriptorSetLayoutBinding> bindingsMap;

    VkGfxDescriptorSetLayout() = delete;

    VkGfxDescriptorSetLayout(const VulkanContext& ctx)
    {
        device = ctx.device;
    }

    CLASS_UNCOPYABLE(VkGfxDescriptorSetLayout);

    /**
     * @brief add resource binding information
     * @param bindingIndex
     * @param descriptorType for the buffer/image resource
     * @param stageFlags
     * @param count, if passing buffer/image arrays
     * @return
     */
    VkGfxDescriptorSetLayout& addBinding(uint32_t           bindingIndex,
                                         VkDescriptorType   descriptorType,
                                         VkShaderStageFlags stageFlags,
                                         uint32_t           count);

    /**
     * @brief create the descriptor set layout
     * @param ctx containing vulkan related information
     * @return result of the create api
     */
    VulkanResult create();

    /**
     * @brief release the descriptor set layout
     */
    void destroy();
};

struct VkGfxDescriptorPool
{
    VkDevice                           device = VK_NULL_HANDLE;
    VkDescriptorPool                   pool   = VK_NULL_HANDLE;
    DynamicArray<VkDescriptorPoolSize> poolSizes {};

    VkGfxDescriptorPool() = delete;
    VkGfxDescriptorPool(const VulkanContext& ctx)
    {
        device = ctx.device;
    }

    CLASS_UNCOPYABLE(VkGfxDescriptorPool);

    /**
     * @brief
     * @param descriptorType type of the descriptor required from the pool
     * @param count for the descriptor of the given type which will be
     * available in the pool
     * @return
     */
    VkGfxDescriptorPool& addPoolSize(VkDescriptorType descriptorType, uint32_t count);

    /**
     * @brief Create the pool for descriptor sets
     * @param maxSets is the maximum number of descriptor sets that can be
     * allocated from the pool.
     * @param poolFlags for pool creation
     * @return result of the api
     */
    VulkanResult create(uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags);

    /**
     * @brief destroy the pool and its resources
     */
    void destroy();

    /**
     * @brief Allocate one descriptor set from the pool
     * @param descriptorSetLayout will be used for the set allocation
     * @param descriptorSet to be allocated
     * @return result of the api
     */
    VulkanResult allocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptorSet) const;

    /**
     * @brief Free on or more descriptor sets
     * @param descriptorSets which are needed to be freed
     */
    void freeDescriptorSets(
        DynamicArray<VkDescriptorSet>& descriptorSets) const;

    /**
     * @brief return all descriptor sets allocated from a given pool, rather than freeing
     * individual sets
     */
    void resetPool();
};

struct VkGfxDescriptorSetWriter
{
};
} // namespace dusk