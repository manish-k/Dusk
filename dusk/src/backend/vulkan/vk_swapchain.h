#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"

namespace dusk
{
struct VkGfxSwapChainParams
{
    uint32_t       windowWidth;
    uint32_t       windowHeight;

    VkSwapchainKHR oldSwapChain;
};

class VkGfxSwapChain
{
public:
    Error          create(VulkanContext& vkContext, VkGfxSwapChainParams& params);
    void           destroy();

    void           resize();

    VkSwapchainKHR getSwapChain() const { return m_swapChain; }

private:
    Error              createSwapChain(const VkGfxSwapChainParams& params);
    void               destroySwapChain();

    VkSurfaceFormatKHR getBestSurfaceFormat() const;
    VkPresentModeKHR   getPresentMode() const;
    VkExtent2D         getSwapExtent(uint32_t width, uint32_t height) const;

private:
    VkPhysicalDevice         m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                 m_device         = VK_NULL_HANDLE;
    VkSurfaceKHR             m_surface        = VK_NULL_HANDLE;
    VkSwapchainKHR           m_swapChain      = VK_NULL_HANDLE;

    VkSurfaceCapabilitiesKHR m_capabilities;

    DynamicArray<VkImage>    m_swapChainImages;

    VkExtent2D               m_currentExtent;
};
} // namespace dusk