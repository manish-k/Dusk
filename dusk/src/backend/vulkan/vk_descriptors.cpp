#include "vk_descriptors.h"

namespace dusk
{

VkGfxDescriptorSetLayout& VkGfxDescriptorSetLayout::addBinding(
    uint32_t           bindingIndex,
    VkDescriptorType   descriptorType,
    VkShaderStageFlags stageFlags,
    uint32_t           count,
    bool               isBindless)
{
    DASSERT(!bindingsMap.has(bindingIndex), "Binding already exists for given index");

    VkDescriptorSetLayoutBinding layoutBinding {};
    layoutBinding.binding         = bindingIndex;
    layoutBinding.descriptorType  = descriptorType;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags      = stageFlags;

    bindingsMap.emplace(bindingIndex, layoutBinding);
    if (isBindless)
    {
        bindingFlags.emplace(
            bindingIndex,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
    }
    else
    {
        bindingFlags.emplace(bindingIndex, 0);
    }

    return *this;
}

VulkanResult VkGfxDescriptorSetLayout::create()
{
    DASSERT(device, "invalid vulkan logical device");

    uint32_t                                   bindingsCount = bindingsMap.size();
    DynamicArray<VkDescriptorSetLayoutBinding> setLayoutBindings(bindingsCount);
    DynamicArray<VkDescriptorBindingFlags>     setBindingFlags(bindingsCount);
    bool                                       isBindless = false;

    for (auto& key : bindingsMap.keys())
    {
        setLayoutBindings[key] = bindingsMap[key];
        setBindingFlags[key]   = bindingFlags[key];

        if (bindingFlags[key]) isBindless = true;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
    bindingFlagsInfo.pNext         = nullptr;
    bindingFlagsInfo.pBindingFlags = setBindingFlags.data();
    bindingFlagsInfo.bindingCount  = setBindingFlags.size();

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    descriptorSetLayoutInfo.pBindings    = setLayoutBindings.data();

    if (isBindless)
    {
        descriptorSetLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        descriptorSetLayoutInfo.pNext = &bindingFlagsInfo;
    }

    return vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &layout);
}

void VkGfxDescriptorSetLayout::destroy()
{
    DASSERT(device, "invalid vulkan logical device");

    vkDestroyDescriptorSetLayout(device, layout, nullptr);
    layout = VK_NULL_HANDLE;
}

VkGfxDescriptorPool& VkGfxDescriptorPool::addPoolSize(VkDescriptorType descriptorType, uint32_t count)
{
    poolSizes.push_back({ descriptorType, count });
    return *this;
}

VulkanResult VkGfxDescriptorPool::create(uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags)
{
    DASSERT(device, "invalid vulkan logical device");

    VkDescriptorPoolCreateInfo descriptorPoolInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes    = poolSizes.data();
    descriptorPoolInfo.maxSets       = maxSets;
    descriptorPoolInfo.flags         = poolFlags;

    return vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &pool);
}

void VkGfxDescriptorPool::destroy()
{
    DASSERT(device, "invalid vulkan logical device");
    DASSERT(pool, "invalid vulkan descriptor pool");

    vkDestroyDescriptorPool(device, pool, nullptr);
    pool = VK_NULL_HANDLE;
}

VulkanResult VkGfxDescriptorPool::allocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet* descriptorSet) const
{
    DASSERT(device, "invalid vulkan logical device");

    VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool     = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &descriptorSetLayout;

    return vkAllocateDescriptorSets(device, &allocInfo, descriptorSet);
}

void VkGfxDescriptorPool::freeDescriptorSets(DynamicArray<VkDescriptorSet>& descriptorSets) const
{
    DASSERT(device, "invalid vulkan logical device");

    vkFreeDescriptorSets(device, pool, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());
}

void VkGfxDescriptorPool::resetPool()
{
    DASSERT(device, "invalid vulkan logical device");
    DASSERT(pool, "invalid vulkan descriptor pool");

    vkResetDescriptorPool(device, pool, 0);
}

VkGfxDescriptorSet& VkGfxDescriptorSet::configureBuffer(
    uint32_t                binding,
    VkDescriptorType        type,
    uint32_t                dstIndex,
    uint32_t                count,
    VkDescriptorBufferInfo* bufferInfo)
{
    DASSERT(setLayout.bindingsMap.has(binding), "Layout doesn't contain specified binding");
    auto& bindingDescription = setLayout.bindingsMap[binding];

    VkWriteDescriptorSet write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.descriptorType  = bindingDescription.descriptorType;
    write.dstBinding      = binding;
    write.pBufferInfo     = bufferInfo;
    write.descriptorCount = count;
    write.descriptorType  = type;
    write.dstArrayElement = dstIndex;

    writes.push_back(write);

    return *this;
}

VkGfxDescriptorSet& VkGfxDescriptorSet::configureImage(
    uint32_t               binding,
    VkDescriptorType       type,
    uint32_t               dstIndex,
    uint32_t               count,
    VkDescriptorImageInfo* imageInfo)
{
    DASSERT(setLayout.bindingsMap.has(binding), "Layout doesn't contain specified binding");
    auto& bindingDescription = setLayout.bindingsMap[binding];

    VkWriteDescriptorSet write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.descriptorType  = bindingDescription.descriptorType;
    write.dstBinding      = binding;
    write.pImageInfo      = imageInfo;
    write.descriptorCount = count;
    write.descriptorType  = type;
    write.dstArrayElement = dstIndex;

    writes.push_back(write);

    return *this;
}

void VkGfxDescriptorSet::applyConfiguration()
{
    if (writes.empty()) return;

    for (auto& write : writes)
    {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    writes.clear();
}

VulkanResult VkGfxDescriptorSet::create()
{
    VulkanResult result = pool.allocateDescriptorSet(setLayout.layout, &set);
    if (!result.isOk())
    {
        DUSK_ERROR("Unable to create descriptor set {}", result.toString());
        return result;
    }

    applyConfiguration();

    return result;
}

} // namespace dusk
