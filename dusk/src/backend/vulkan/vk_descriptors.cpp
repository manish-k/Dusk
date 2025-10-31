#include "vk_descriptors.h"

namespace dusk
{

VkGfxDescriptorSetLayout::Builder& VkGfxDescriptorSetLayout::Builder::addBinding(
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

VkGfxDescriptorSetLayout::Builder& VkGfxDescriptorSetLayout::Builder::setDebugName(const std::string& name)
{
    debugName = name;
    return *this;
}

Unique<VkGfxDescriptorSetLayout> VkGfxDescriptorSetLayout::Builder::build()
{
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

    auto         gfxSetLayout = createUnique<VkGfxDescriptorSetLayout>(device, bindingsMap);
    VulkanResult result       = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &gfxSetLayout->layout);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create descriptor set layout. {}", result.toString());
        return nullptr;
    }

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        device,
        VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
        (uint64_t)gfxSetLayout->layout,
        debugName.c_str());
#endif // VK_RENDERER_DEBUG

    return std::move(gfxSetLayout);
}

VkGfxDescriptorSetLayout::~VkGfxDescriptorSetLayout()
{
    vkDestroyDescriptorSetLayout(device, layout, nullptr);
}

VkGfxDescriptorPool::Builder& VkGfxDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, uint32_t count)
{
    poolSizes.push_back({ descriptorType, count });
    return *this;
}

VkGfxDescriptorPool::Builder& VkGfxDescriptorPool::Builder::setDebugName(const std::string& name)
{
    debugName = name;
    return *this;
}

Unique<VkGfxDescriptorPool> VkGfxDescriptorPool::Builder::build(uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags)
{
    DASSERT(device, "invalid vulkan logical device");

    VkDescriptorPoolCreateInfo descriptorPoolInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes    = poolSizes.data();
    descriptorPoolInfo.maxSets       = maxSets;
    descriptorPoolInfo.flags         = poolFlags;

    auto         gfxDescriptorPool   = createUnique<VkGfxDescriptorPool>(device);
    VulkanResult result              = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &gfxDescriptorPool->pool);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to build descriptor pool. {}", result.toString());
        return nullptr;
    }

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        device,
        VK_OBJECT_TYPE_DESCRIPTOR_POOL,
        (uint64_t)gfxDescriptorPool->pool,
        debugName.c_str());
#endif // VK_RENDERER_DEBUG

    return std::move(gfxDescriptorPool);
}

VkGfxDescriptorPool::~VkGfxDescriptorPool()
{
    vkDestroyDescriptorPool(device, pool, nullptr);
}

Unique<VkGfxDescriptorSet> VkGfxDescriptorPool::allocateDescriptorSet(
    VkGfxDescriptorSetLayout& descriptorSetLayout,
    const char*               pDebugName)
{
    VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool     = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &descriptorSetLayout.layout;

    auto         descriptorSet   = createUnique<VkGfxDescriptorSet>(device, descriptorSetLayout);

    VulkanResult result          = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet->set);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create descriptor set {}. {}", pDebugName, result.toString());
        return nullptr;
    }

#ifdef VK_RENDERER_DEBUG
    if (pDebugName)
    {
        vkdebug::setObjectName(
            device,
            VK_OBJECT_TYPE_DESCRIPTOR_SET,
            (uint64_t)descriptorSet->set,
            pDebugName);
    }

#endif // VK_RENDERER_DEBUG

    return std::move(descriptorSet);
}

void VkGfxDescriptorPool::freeDescriptorSets(DynamicArray<VkDescriptorSet>& descriptorSets) const
{
    vkFreeDescriptorSets(device, pool, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());
}

void VkGfxDescriptorPool::resetPool()
{
    DASSERT(pool, "invalid vulkan descriptor pool");

    vkResetDescriptorPool(device, pool, 0);
}

VkGfxDescriptorSet& VkGfxDescriptorSet::configureBuffer(
    uint32_t                binding,
    uint32_t                dstIndex,
    uint32_t                count,
    VkDescriptorBufferInfo* bufferInfo)
{
    DASSERT(setLayout.bindingsMap.has(binding), "Layout doesn't contain specified binding");
    auto&                bindingDescription = setLayout.bindingsMap[binding];

    VkWriteDescriptorSet write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.descriptorType  = bindingDescription.descriptorType;
    write.dstBinding      = binding;
    write.pBufferInfo     = bufferInfo;
    write.descriptorCount = count;
    write.descriptorType  = bindingDescription.descriptorType;
    write.dstArrayElement = dstIndex;

    writes.push_back(write);

    return *this;
}

VkGfxDescriptorSet& VkGfxDescriptorSet::configureImage(
    uint32_t               binding,
    uint32_t               dstIndex,
    uint32_t               count,
    VkDescriptorImageInfo* imageInfo)
{
    DASSERT(setLayout.bindingsMap.has(binding), "Layout doesn't contain specified binding");
    auto&                bindingDescription = setLayout.bindingsMap[binding];

    VkWriteDescriptorSet write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.descriptorType  = bindingDescription.descriptorType;
    write.dstBinding      = binding;
    write.pImageInfo      = imageInfo;
    write.descriptorCount = count;
    write.descriptorType  = bindingDescription.descriptorType;
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

} // namespace dusk
