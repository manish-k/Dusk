#include "ui.h"

#include "engine.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace dusk
{
bool UI::init(Window& window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // We are using Docking Branch

    // init window
    ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window.getNativeWindow(), true);

    // get current vulkan context
    auto& ctx       = VkGfxDevice::getSharedVulkanContext();
    auto& swapChain = Engine::get().getRenderer().getSwapChain();

    // create descriptor pool
    m_descriptorPool    = createUnique<VkGfxDescriptorPool>(ctx);
    VulkanResult result = m_descriptorPool
                              ->addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                              .create(1, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create descriptor pool for ImGui {}", result.toString());
        return false;
    }

    // for dynamic rendering
    DynamicArray<VkFormat>        colorAttachmentFormats { swapChain.getImageFormat() };
    VkPipelineRenderingCreateInfo renderingCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    renderingCreateInfo.colorAttachmentCount    = static_cast<size_t>(colorAttachmentFormats.size());
    renderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
    renderingCreateInfo.depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT_S8_UINT;
    renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

    // setup imgui with out backend
    ImGui_ImplVulkan_InitInfo init_info   = {};
    init_info.Instance                    = ctx.vulkanInstance;
    init_info.PhysicalDevice              = ctx.physicalDevice;
    init_info.Device                      = ctx.device;
    init_info.QueueFamily                 = ctx.graphicsQueueFamilyIndex;
    init_info.Queue                       = ctx.graphicsQueue;
    init_info.PipelineCache               = VK_NULL_HANDLE;
    init_info.DescriptorPool              = m_descriptorPool->pool;
    init_info.RenderPass                  = VK_NULL_HANDLE;
    init_info.UseDynamicRendering         = VK_TRUE;
    init_info.PipelineRenderingCreateInfo = renderingCreateInfo;
    init_info.Subpass                     = 0;
    init_info.MinImageCount               = swapChain.getImagesCount() - 1;
    init_info.ImageCount                  = swapChain.getImagesCount();
    init_info.MSAASamples                 = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator                   = VK_NULL_HANDLE;
    init_info.CheckVkResultFn             = checkVkResult;

    ImGui_ImplVulkan_Init(&init_info);
}

void UI::shutdown()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_descriptorPool->resetPool();
    m_descriptorPool->destroy();
    m_descriptorPool = nullptr;
}

void UI::beginRendering()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();
}

void UI::endRendering(VkCommandBuffer cb)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
}

void UI::checkVkResult(VkResult err)
{
    VulkanResult result(err);

    if (result.isOk()) return;

    DUSK_ERROR("Imgui vulkan error: {}", result.toString());
}

} // namespace dusk