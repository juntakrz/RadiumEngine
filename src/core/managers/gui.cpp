#include "pch.h"
#include "util/util.h"
#include "core/core.h"
#include "core/managers/window.h"
#include "core/managers/renderer.h"
#include "core/managers/resources.h"
#include "core/managers/gui.h"

core::MGUI::MGUI() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  m_pIO = &ImGui::GetIO();
}

void core::MGUI::initialize() {
  RE_LOG(Log, "Initializing GUI manager.");

  // Connect imGUI to GLFW
  ImGui_ImplGlfw_InitForVulkan(core::window.getWindow(), true);

  // Prepare pipeline info for imGUI to use in a dynamic rendering pass later
  RTexture* pTexture = core::resources.getTexture(RTGT_IMGUI);

  VkPipelineRenderingCreateInfoKHR pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  pipelineInfo.viewMask = NULL;
  pipelineInfo.colorAttachmentCount = 1;
  pipelineInfo.pColorAttachmentFormats = &core::renderer.swapchain.pImages[0]->texture.imageFormat;
  pipelineInfo.pNext = nullptr;
  
  ImGui_ImplVulkan_InitInfo vulkanInfo{};
  vulkanInfo.Instance = core::renderer.APIInstance;
  vulkanInfo.PhysicalDevice = core::renderer.physicalDevice.device;
  vulkanInfo.Device = core::renderer.logicalDevice.device;
  vulkanInfo.DescriptorPool = core::renderer.system.descriptorPool;
  vulkanInfo.Queue = core::renderer.logicalDevice.queues.graphics;
  vulkanInfo.QueueFamily =
    static_cast<uint32_t>(core::renderer.physicalDevice.queueFamilyIndices.graphics[0]);
  vulkanInfo.PipelineCache = VK_NULL_HANDLE;
  vulkanInfo.ImageCount = core::renderer.swapchain.imageCount;
  vulkanInfo.MinImageCount = vulkanInfo.ImageCount;
  vulkanInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  vulkanInfo.UseDynamicRendering = true;
  vulkanInfo.PipelineRenderingCreateInfo = pipelineInfo;
  vulkanInfo.RenderPass = VK_NULL_HANDLE;
  vulkanInfo.Subpass = 0u;
  //vulkanInfo.Allocator = &core::renderer.memAlloc->m_AllocationCallbacks;
  vulkanInfo.Allocator = VK_NULL_HANDLE;
  vulkanInfo.CheckVkResultFn = nullptr;

  ImGui_ImplVulkan_Init(&vulkanInfo);

  ImGui_ImplVulkan_CreateFontsTexture();

  // Prepare dynamic rendering data
  core::renderer.gui.pTexture = pTexture;

  VkRenderingAttachmentInfo& attachmentInfo = core::renderer.gui.colorAttachment;
  attachmentInfo = VkRenderingAttachmentInfo{};
  attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  attachmentInfo.clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};
  attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentInfo.imageView = pTexture->texture.view;
  attachmentInfo.imageLayout = pTexture->texture.imageLayout;
  attachmentInfo.pNext = nullptr;

  VkRenderingInfo& renderingInfo = core::renderer.gui.renderingInfo;
  renderingInfo = VkRenderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.pColorAttachments = &attachmentInfo;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.layerCount = pTexture->texture.layerCount;
  renderingInfo.renderArea.offset = { 0, 0 };
  renderingInfo.renderArea.extent.width = pTexture->texture.width;
  renderingInfo.renderArea.extent.height = pTexture->texture.height;
  renderingInfo.pDepthAttachment = nullptr;
  renderingInfo.pStencilAttachment = nullptr;
  renderingInfo.viewMask = NULL;
  renderingInfo.flags = NULL;
  renderingInfo.pNext = nullptr;
}

void core::MGUI::deinitialize() {
  ImGui_ImplVulkan_DestroyFontsTexture();
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
}

ImGuiIO& core::MGUI::io() {
  return *m_pIO;
}
