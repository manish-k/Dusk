#include "editor_ui.h"

#include "engine.h"
#include "scene_widgets.h"
#include "renderer_widgets.h"
#include "stats_widgets.h"
#include "events/key_event.h"
#include "backend/vulkan/vk_renderer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <string_view>

namespace dusk
{
constexpr char imguiConfigFile[] = CONCAT(STRING(DUSK_BUILD_PATH), "/dusk_imgui.ini");

UIState        EditorUI::s_uiState     = {};

bool           EditorUI::init(Window& window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // We are using Docking Branch
    io.IniFilename = imguiConfigFile;

    setupImGuiTheme();

    // init window
    ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window.getNativeWindow(), true);

    // get current vulkan context
    auto& ctx       = VkGfxDevice::getSharedVulkanContext();
    auto& swapChain = Engine::get().getRenderer().getSwapChain();

    // create descriptor pool
    m_descriptorPool = VkGfxDescriptorPool::Builder(ctx)
                           .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                           .setDebugName("ui_desc_pool")
                           .build(1, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
    CHECK_AND_RETURN_FALSE(!m_descriptorPool);

    // for dynamic rendering
    DynamicArray<VkFormat>        colorAttachmentFormats { swapChain.getImageFormat() };
    VkPipelineRenderingCreateInfo renderingCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    renderingCreateInfo.colorAttachmentCount    = static_cast<size_t>(colorAttachmentFormats.size());
    renderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats.data();

    // setup imgui with our backend
    ImGui_ImplVulkan_InitInfo initInfo   = {};
    initInfo.Instance                    = ctx.vulkanInstance;
    initInfo.PhysicalDevice              = ctx.physicalDevice;
    initInfo.Device                      = ctx.device;
    initInfo.QueueFamily                 = ctx.graphicsQueueFamilyIndex;
    initInfo.Queue                       = ctx.graphicsQueue;
    initInfo.PipelineCache               = VK_NULL_HANDLE;
    initInfo.DescriptorPool              = m_descriptorPool->pool;
    initInfo.RenderPass                  = VK_NULL_HANDLE;
    initInfo.UseDynamicRendering         = VK_TRUE;
    initInfo.PipelineRenderingCreateInfo = renderingCreateInfo;
    initInfo.Subpass                     = 0;
    initInfo.MinImageCount               = swapChain.getImagesCount() - 1;
    initInfo.ImageCount                  = swapChain.getImagesCount();
    initInfo.MSAASamples                 = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator                   = VK_NULL_HANDLE;
    initInfo.CheckVkResultFn             = checkVkResult;

    ImGui_ImplVulkan_Init(&initInfo);

    return true;
}

void EditorUI::shutdown()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_descriptorPool = nullptr;
}

void EditorUI::beginRendering()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void EditorUI::renderCommonWidgets()
{
    drawStatsWidget();
    drawRendererWidget();
    // ImGui::ShowDemoWindow();
}

void EditorUI::endRendering(VkCommandBuffer cb)
{
    ImGui::Render();

    if (m_isShowing)
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
}

void EditorUI::renderSceneWidgets(Scene& scene)
{
    drawSceneGraphWidget(scene);
}

void EditorUI::onEvent(Event& ev)
{
    EventDispatcher dispatcher(ev);

    dispatcher.dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent& ev)
        {
            if (ev.getKeyCode() == Key::F4)
            {
                m_isShowing = !m_isShowing;
            }

            return false;
        });
}

void EditorUI::setupImGuiTheme()
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

void EditorUI::checkVkResult(VkResult err)
{
    VulkanResult result(err);

    if (result.isOk()) return;

    DUSK_ERROR("Imgui vulkan error: {}", result.toString());
}

} // namespace dusk