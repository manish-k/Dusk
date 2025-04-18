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
    HashMap<uint32_t, VkDescriptorBindingFlags>     bindingFlags {};

    VkGfxDescriptorSetLayout() = delete;

    explicit VkGfxDescriptorSetLayout(const VulkanContext& ctx)
    {
        device = ctx.device;
    }

    CLASS_UNCOPYABLE(VkGfxDescriptorSetLayout);

    /**
     * @brief add resource binding information
     * @param bindingIndex
     * @param descriptorType for the buffer/image resource
     * @param stageFlags
     * @param count if passing buffer/image arrays
     * @param isBindless whether this binding is bindless or not
     * @return
     */
    VkGfxDescriptorSetLayout& addBinding(
        uint32_t           bindingIndex,
        VkDescriptorType   descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t           count,
        bool               isBindless = false);

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
    bool                               isBindless = false;

    VkGfxDescriptorPool()                         = delete;
    explicit VkGfxDescriptorPool(const VulkanContext& ctx)
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
    VulkanResult create(uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags = 0);

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
    VulkanResult allocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet* descriptorSet) const;

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

struct VkGfxDescriptorSet
{
    VkDevice                           device = VK_NULL_HANDLE;
    VkDescriptorSet                    set    = VK_NULL_HANDLE;

    VkGfxDescriptorPool&               pool;
    VkGfxDescriptorSetLayout&          setLayout;
    DynamicArray<VkWriteDescriptorSet> writes;

    VkGfxDescriptorSet() = delete;
    explicit VkGfxDescriptorSet(
        const VulkanContext&            ctx,
        VkGfxDescriptorSetLayout& layout,
        VkGfxDescriptorPool&      pool) :
        device(ctx.device), pool(pool), setLayout(layout)
    {
    }

    /**
     * @brief Configure buffer which is part of descriptor
     * @param binding number in the set
     * @param dstIndex index of the element which has to be configured
     * @param count number of descriptors to update
     * @param bufferInfo contains buffer details for linking to the descriptor
     * @return gfx descriptor set
     */
    VkGfxDescriptorSet& configureBuffer(
        uint32_t                binding,
        VkDescriptorType        type,
        uint32_t                dstIndex,
        uint32_t                count,
        VkDescriptorBufferInfo* bufferInfo);

    /**
     * @brief Configure image which is part of descriptor
     * @param binding number in the set
     * @param type
     * @param dstIndex index of the element which has to be configured
     * @param count number of descriptors to update
     * @param imageInfo contains image deatils for linking to the descriptor
     * @return gfx descriptor set
     */
    VkGfxDescriptorSet& configureImage(
        uint32_t               binding,
        VkDescriptorType       type,
        uint32_t               dstIndex,
        uint32_t               count,
        VkDescriptorImageInfo* imageInfo);

    /**
     * @brief apply buffer/image configurations on the set. It will overwrite any
     * existing configurations
     */
    void applyConfiguration();

    /**
     * @brief create the descriptor set and apply any give configurations
     * @return vulkan result of the api command
     */
    VulkanResult create();
};

} // namespace dusk