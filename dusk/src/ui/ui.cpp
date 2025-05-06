#include "ui.h"

#include "engine.h"
#include "scene_widgets.h"

#include <imgui.h>
#include <imgui_internal.h>
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

    setupImGuiTheme();

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

    // ImGui::ShowDemoWindow();
}

void UI::endRendering(VkCommandBuffer cb)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
}

void UI::renderSceneWidgets(Scene& scene)
{
    drawSceneGraphWidget(scene);
}

void UI::switchUIDisplay(bool state)
{
    m_isShowing = state;
}

void UI::setupImGuiTheme()
{
    // TODO:: enable alpha blending in the backend

    ImGuiStyle& style                      = ImGui::GetStyle();
    style.WindowRounding                   = 3;
    style.GrabRounding                     = 2;
    style.FrameRounding                    = 3;
    style.FrameBorderSize                  = 0;
    style.WindowBorderSize                 = 0;
    style.DockingSeparatorSize             = 1;
    style.WindowPadding                    = { 8, 5 };

    auto&        colors                    = ImGui::GetStyle().Colors;
    const ImVec4 almostBlack               = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    const ImVec4 darkGray                  = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    const ImVec4 midGray                   = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    const ImVec4 lightGray                 = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    const ImVec4 textColor                 = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

    colors[ImGuiCol_Text]                  = textColor;
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]              = almostBlack;
    colors[ImGuiCol_ChildBg]               = darkGray;
    colors[ImGuiCol_PopupBg]               = almostBlack;
    colors[ImGuiCol_Border]                = midGray;
    colors[ImGuiCol_FrameBg]               = midGray;
    colors[ImGuiCol_FrameBgHovered]        = lightGray;
    colors[ImGuiCol_FrameBgActive]         = lightGray;
    colors[ImGuiCol_TitleBg]               = darkGray;
    colors[ImGuiCol_TitleBgActive]         = midGray;
    colors[ImGuiCol_TitleBgCollapsed]      = almostBlack;
    colors[ImGuiCol_MenuBarBg]             = darkGray;
    colors[ImGuiCol_ScrollbarBg]           = darkGray;
    colors[ImGuiCol_ScrollbarGrab]         = midGray;
    colors[ImGuiCol_ScrollbarGrabHovered]  = lightGray;
    colors[ImGuiCol_ScrollbarGrabActive]   = lightGray;
    colors[ImGuiCol_CheckMark]             = textColor;
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_Button]                = midGray;
    colors[ImGuiCol_ButtonHovered]         = lightGray;
    colors[ImGuiCol_ButtonActive]          = lightGray;
    colors[ImGuiCol_Header]                = midGray;
    colors[ImGuiCol_HeaderHovered]         = lightGray;
    colors[ImGuiCol_HeaderActive]          = lightGray;
    colors[ImGuiCol_Separator]             = midGray;
    colors[ImGuiCol_SeparatorHovered]      = lightGray;
    colors[ImGuiCol_SeparatorActive]       = lightGray;
    colors[ImGuiCol_ResizeGrip]            = midGray;
    colors[ImGuiCol_ResizeGripHovered]     = lightGray;
    colors[ImGuiCol_ResizeGripActive]      = lightGray;
    colors[ImGuiCol_Tab]                   = midGray;
    colors[ImGuiCol_TabHovered]            = lightGray;
    colors[ImGuiCol_TabActive]             = lightGray;
    colors[ImGuiCol_TabUnfocused]          = darkGray;
    colors[ImGuiCol_TabUnfocusedActive]    = midGray;
    colors[ImGuiCol_TabSelectedOverline]   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_DockingPreview]        = lightGray;
    colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = lightGray;
    colors[ImGuiCol_PlotHistogram]         = midGray;
    colors[ImGuiCol_PlotHistogramHovered]  = lightGray;
    colors[ImGuiCol_TextSelectedBg]        = midGray;
    colors[ImGuiCol_DragDropTarget]        = lightGray;
    colors[ImGuiCol_NavHighlight]          = lightGray;
    colors[ImGuiCol_NavWindowingHighlight] = lightGray;
    colors[ImGuiCol_NavWindowingDimBg]     = darkGray;
    colors[ImGuiCol_ModalWindowDimBg]      = almostBlack;
}

void UI::checkVkResult(VkResult err)
{
    VulkanResult result(err);

    if (result.isOk()) return;

    DUSK_ERROR("Imgui vulkan error: {}", result.toString());
}

} // namespace dusk