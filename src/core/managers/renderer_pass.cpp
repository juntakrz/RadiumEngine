#include "pch.h"
#include "core/core.h"
#include "core/material/texture.h"
#include "core/managers/renderer.h"

TResult core::MRenderer::createRenderPass(ERenderPass renderPassId, EPipeline pipeline, RRenderPassInfo* info) {
  // If render pass already exists - don't proceed with its creation, but only add pipeline entry if not present as well
  const bool renderPassExists = !system.renderPasses.try_emplace(renderPassId).second;
  RRenderPass* pRenderPass = &system.renderPasses.at(renderPassId);
  RPipeline* pPipeline = system.renderPasses.at(renderPassId).getPipeline(pipeline);

  if (!pPipeline) {
    pPipeline = &system.graphicsPipelines[pipeline];
    pPipeline->pipelineId = pipeline;
    pRenderPass->pipelines.emplace_back(pPipeline);
  }

  pPipeline->subpassIndex = info->subpassIndex;

  pRenderPass->layout = getPipelineLayout(info->layout);
  pRenderPass->clearValues = info->clearValues;

  if (renderPassExists) {
    return RE_OK;
  }

  RE_LOG(Log, "Creating render pass E%d", renderPassId);

  // put all the color and the depth attachments in the same buffer
  const uint32_t colorAttachmentCount = static_cast<uint32_t>(info->colorAttachmentInfo.size());
  const bool isDepthAttachmentValid = info->depthAttachmentInfo.samples != NULL;
  VkAttachmentDescription attachments[10];
  assert(info->colorAttachmentInfo.size() < 10);

  memcpy(attachments, info->colorAttachmentInfo.data(), sizeof(VkAttachmentDescription) * colorAttachmentCount);

  if (isDepthAttachmentValid) {
    memcpy(&attachments[colorAttachmentCount], &info->depthAttachmentInfo, sizeof(VkAttachmentDescription));
  }

  // create references for the attachments
  VkAttachmentReference colorReference[10];
  for (uint32_t i = 0; i < colorAttachmentCount; i++)
    colorReference[i] = { i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

  VkAttachmentReference depthReference = {
      colorAttachmentCount, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

  std::vector<VkSubpassDescription> subpassDescriptions;
  std::vector<VkSubpassDependency> subpassDependencies;

  switch (renderPassId) {
  case ERenderPass::Deferred: {
    /* Create deferred subpass

    Last two color references are used in later subpasses*/
    VkSubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.flags = 0;
    subpassDesc.inputAttachmentCount = 0;
    subpassDesc.pInputAttachments = NULL;
    subpassDesc.colorAttachmentCount = colorAttachmentCount - 1;
    subpassDesc.pColorAttachments = colorReference;
    subpassDesc.pResolveAttachments = NULL;
    subpassDesc.pDepthStencilAttachment = &depthReference;
    subpassDesc.preserveAttachmentCount = 0;
    subpassDesc.pPreserveAttachments = NULL;

    subpassDescriptions.emplace_back(subpassDesc);

    /* Create PBR subpass

    Convert G-buffer color attachments to shader read sources */
    VkAttachmentReference colorReferencePBR[10];
    for (uint32_t j = 0; j < colorAttachmentCount - 1; ++j) {
      colorReferencePBR[j] = { j, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    }

    // Use deferred color references as input attachments
    // and use next to last color reference as PBR attachment
    subpassDesc = VkSubpassDescription{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.flags = 0;
    subpassDesc.inputAttachmentCount = colorAttachmentCount - 1;
    subpassDesc.pInputAttachments = colorReferencePBR;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorReference[colorAttachmentCount - 1];
    subpassDesc.pResolveAttachments = NULL;
    subpassDesc.pDepthStencilAttachment = NULL;
    subpassDesc.preserveAttachmentCount = 0;
    subpassDesc.pPreserveAttachments = NULL;

    subpassDescriptions.emplace_back(subpassDesc);

    /* Create Forward subpass

    Using depth target from subpass 0 render objects that do not fit deferred pipeline */
    subpassDesc = VkSubpassDescription{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.flags = 0;
    subpassDesc.inputAttachmentCount = 0;
    subpassDesc.pInputAttachments = NULL;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorReference[colorAttachmentCount - 1];
    subpassDesc.pResolveAttachments = NULL;
    subpassDesc.pDepthStencilAttachment = &depthReference;
    subpassDesc.preserveAttachmentCount = 0;
    subpassDesc.pPreserveAttachments = NULL;

    subpassDescriptions.emplace_back(subpassDesc);

    // Create deferred dependency
    VkSubpassDependency subpassDependency{};
    subpassDependency.dependencyFlags = 0;
    subpassDependency.srcAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpassDependency.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpassDependency.srcSubpass = 0;
    subpassDependency.dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpassDependency.dstStageMask =
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpassDependency.dstSubpass = 1;

    subpassDependencies.emplace_back(subpassDependency);

    // Create PBR dependency
    subpassDependency.srcSubpass = 1;
    subpassDependency.dstSubpass = 2;
    subpassDependency.dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    subpassDependencies.emplace_back(subpassDependency);

    // Create Forward dependency
    subpassDependency.srcSubpass = 2;
    subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;

    subpassDependencies.emplace_back(subpassDependency);

    break;
  }

  default: {
    // Create subpass
    VkSubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.flags = 0;
    subpassDesc.inputAttachmentCount = 0;
    subpassDesc.pInputAttachments = NULL;
    subpassDesc.colorAttachmentCount = colorAttachmentCount;
    subpassDesc.pColorAttachments = colorReference;
    subpassDesc.pResolveAttachments = NULL;
    subpassDesc.pDepthStencilAttachment = (isDepthAttachmentValid) ? &depthReference : NULL;
    subpassDesc.preserveAttachmentCount = 0;
    subpassDesc.pPreserveAttachments = NULL;

    VkSubpassDependency subpassDependency{};
    subpassDependency.dependencyFlags = 0;
    subpassDependency.srcAccessMask =
      ((colorAttachmentCount) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
      ((isDepthAttachmentValid) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        : 0);
    subpassDependency.srcStageMask =
      ((colorAttachmentCount)
        ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        : 0) |
      ((isDepthAttachmentValid) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        : 0);
    subpassDependency.srcSubpass = 0;
    subpassDependency.dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT |
      ((colorAttachmentCount) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
      ((isDepthAttachmentValid) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        : 0);
    subpassDependency.dstStageMask =
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
      ((colorAttachmentCount)
        ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        : 0) |
      ((isDepthAttachmentValid) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        : 0);
    subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;

    subpassDescriptions.emplace_back(subpassDesc);
    subpassDependencies.emplace_back(subpassDependency);

    break;
  }
  }

  // Create render pass
  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.pNext = NULL;
  renderPassInfo.attachmentCount = colorAttachmentCount;
  if (isDepthAttachmentValid) renderPassInfo.attachmentCount++;
  renderPassInfo.pAttachments = attachments;
  renderPassInfo.subpassCount =
    static_cast<uint32_t>(subpassDescriptions.size());
  renderPassInfo.pSubpasses = subpassDescriptions.data();
  renderPassInfo.dependencyCount =
    static_cast<uint32_t>(subpassDependencies.size());
  renderPassInfo.pDependencies = subpassDependencies.data();

  VkRenderPass newRenderPass;
  VkResult result =
    vkCreateRenderPass(logicalDevice.device, &renderPassInfo, NULL, &newRenderPass);
  assert(result == VK_SUCCESS);

  pRenderPass->renderPass = newRenderPass;

  /*setResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass,
                  "CreateRenderPass");*/

  return RE_OK;
}

TResult core::MRenderer::createDynamicRenderPass(EDynamicRenderingPass passId, RDynamicRenderingInfo* pInfo) {
  VkRenderingAttachmentInfo defaultAttachment{};
  defaultAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  defaultAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  defaultAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  defaultAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  defaultAttachment.clearValue = { 1.0f, 0.0f, 0.0f, 0.0f };

  // Create or reconfigure the dynamic render pass
  RDynamicRenderingPass* pRenderPass = &system.dynamicRenderingPasses[passId];
  const uint32_t colorAttachmentCount = static_cast<uint32_t>(pInfo->colorViews.size());

  // Set dynamic rendering pass
  pRenderPass->passId = passId;
  pRenderPass->viewportId = pInfo->viewportId;

  pRenderPass->renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  pRenderPass->renderingInfo.renderArea = getViewportData(pRenderPass->viewportId)->scissor;
  pRenderPass->renderingInfo.layerCount = 1u;
  pRenderPass->renderingInfo.flags = NULL;
  pRenderPass->renderingInfo.viewMask = NULL;

  if (colorAttachmentCount) {
    pRenderPass->colorAttachments.resize(colorAttachmentCount);
    
    for (uint8_t attachmentIndex = 0; attachmentIndex < colorAttachmentCount; ++attachmentIndex) {
      pRenderPass->colorAttachments[attachmentIndex] = defaultAttachment;
      pRenderPass->colorAttachments[attachmentIndex].imageView = pInfo->colorViews[attachmentIndex].first;
      pRenderPass->colorAttachments[attachmentIndex].clearValue = pInfo->colorAttachmentClearValue;
    }

    pRenderPass->renderingInfo.pColorAttachments = pRenderPass->colorAttachments.data();
    pRenderPass->renderingInfo.colorAttachmentCount = colorAttachmentCount;
  }

  if (pInfo->depthView.first) {
    pRenderPass->depthAttachment = defaultAttachment;
    pRenderPass->depthAttachment.imageView = pInfo->depthView.first;
    pRenderPass->depthAttachment.imageLayout = pInfo->depthLayout;

    pRenderPass->renderingInfo.pDepthAttachment = &pRenderPass->depthAttachment;
  }

  if (pInfo->stencilView.first) {
    pRenderPass->stencilAttachment = defaultAttachment;
    pRenderPass->stencilAttachment.imageView = pInfo->stencilView.first;
    pRenderPass->stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;

    pRenderPass->renderingInfo.pStencilAttachment = &pRenderPass->stencilAttachment;
  }

  RPipeline* pPipeline = getGraphicsPipeline(pInfo->pipeline);

  if (pPipeline) {
    pRenderPass->pPipeline = pPipeline;

    RE_LOG(Log, "Created dynamic rendering pass E%d using an already existing pipeline E%d.", passId, pInfo->pipeline);
    return RE_OK;
  }

  pPipeline = &system.graphicsPipelines[pInfo->pipeline];
  pPipeline->pipeline = VK_NULL_HANDLE;
  pPipeline->pipelineId = pInfo->pipeline;
  pPipeline->subpassIndex = 0;
  pPipeline->layoutId = pInfo->pipelineLayout;
  pPipeline->layout = getPipelineLayout(pPipeline->layoutId);

  std::vector<VkFormat> colorAttachmentFormats(colorAttachmentCount);

  for (uint32_t j = 0; j < colorAttachmentCount; ++j) {
    colorAttachmentFormats[j] = pInfo->colorViews[j].second;
  }

  // this data will be used during graphics pipeline creation
  VkPipelineRenderingCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  pipelineCreateInfo.viewMask = 0;

  if (colorAttachmentCount) {
    pipelineCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentCount);
    pipelineCreateInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
  }

  if (pInfo->depthView.first) {
    pipelineCreateInfo.depthAttachmentFormat = pInfo->depthView.second;
  }

  if (pInfo->stencilView.first) {
    pipelineCreateInfo.stencilAttachmentFormat = pInfo->stencilView.second;
  }

  RGraphicsPipelineInfo pipelineInfo{};
  pipelineInfo.pipeline = pInfo->pipeline;
  pipelineInfo.pipelineLayout = pInfo->pipelineLayout;
  pipelineInfo.renderPass = ERenderPass::Null;
  pipelineInfo.dynamicRenderPass = passId;
  pipelineInfo.pDynamicPipelineInfo = &pipelineCreateInfo;
  pipelineInfo.vertexShader = pInfo->vertexShader;
  pipelineInfo.fragmentShader = pInfo->fragmentShader;
  pipelineInfo.colorBlendAttachmentCount = colorAttachmentCount;
  pipelineInfo.viewportId = pInfo->viewportId;

  RE_CHECK(createGraphicsPipeline(&pipelineInfo));

  pRenderPass->pPipeline = pPipeline;

  RE_LOG(Log, "Create dynamic rendering pass E%d and graphics pipeline E%d.", passId, pInfo->pipeline);
  return RE_OK;
}

TResult core::MRenderer::createRenderPasses() {
  // lambda for quickly filling common clear values for render targets
  auto fGetClearValues = [](VkClearColorValue colorValue,
    int8_t colorValueCount,
    VkClearDepthStencilValue depthValue) {
      std::vector<VkClearValue> clearValues;
      clearValues.resize(colorValueCount + 1);

      for (int8_t i = 0; i < colorValueCount; ++i) {
        clearValues[i].color = colorValue;
      }

      clearValues[colorValueCount].depthStencil = depthValue;
      return clearValues;
    };

  // standard info setup
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapchain.formatData.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp =
    VK_ATTACHMENT_LOAD_OP_CLEAR;  // clearing contents on new frame
  colorAttachment.storeOp =
    VK_ATTACHMENT_STORE_OP_STORE; // storing contents in memory while rendering
  colorAttachment.stencilLoadOp =
    VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // ignore stencil buffer
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout =
    VK_IMAGE_LAYOUT_UNDEFINED;    // not important, since it's cleared at the
  // frame start
  colorAttachment.finalLayout =
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // swap chain image to be presented

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = core::vulkan::formatDepth;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  // common clear color and clear depth values
  const VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
  const VkClearDepthStencilValue clearDepth = { 1.0f };

  //
  // DEFERRED render pass
  // 
  {
    // all models are rendered to 5 separate color maps forming G-buffer
    // then they are combined into 1 PBR color attachment
    colorAttachment.format = core::vulkan::formatHDR16;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // deferred render targets: position, color, normal, physical, emissive
    // + PBR render target
    uint32_t attachmentCount = 6;
    std::vector<VkAttachmentDescription> deferredColorAttachments(attachmentCount, colorAttachment);

    RRenderPassInfo passInfo{};
    passInfo.colorAttachmentInfo = deferredColorAttachments;
    passInfo.depthAttachmentInfo = depthAttachment;
    passInfo.layout = EPipelineLayout::Scene;
    passInfo.viewportWidth = config::renderWidth;
    passInfo.viewportHeight = config::renderHeight;
    passInfo.clearValues = fGetClearValues(clearColor, attachmentCount, clearDepth);
    passInfo.subpassIndex = 0;

    createRenderPass(ERenderPass::Deferred, EPipeline::OpaqueCullBack, &passInfo);
    createRenderPass(ERenderPass::Deferred, EPipeline::OpaqueCullNone, &passInfo);
    createRenderPass(ERenderPass::Deferred, EPipeline::BlendCullNone, &passInfo);
    // TODO: add mask pipeline

    passInfo.subpassIndex = 1;

    createRenderPass(ERenderPass::Deferred, EPipeline::PBR, &passInfo);

    passInfo.subpassIndex = 2;

    createRenderPass(ERenderPass::Deferred, EPipeline::Skybox, &passInfo);
  }

  //
  // PRESENT render pass
  // 
  {
    // the 16 bit float color map is rendered to the compatible surface target
    colorAttachment.format = swapchain.formatData.format;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    RRenderPassInfo passInfo{};
    passInfo.colorAttachmentInfo = { colorAttachment };
    passInfo.depthAttachmentInfo = depthAttachment;
    passInfo.layout = EPipelineLayout::Scene;
    passInfo.viewportWidth = config::renderWidth;
    passInfo.viewportHeight = config::renderHeight;
    passInfo.clearValues = fGetClearValues(clearColor, 1, clearDepth);
    passInfo.subpassIndex = 0;

    createRenderPass(ERenderPass::Present, EPipeline::Present, &passInfo);
  }

  //
  // SHADOW render pass
  //
  {
    depthAttachment.format = core::vulkan::formatShadow;

    RRenderPassInfo passInfo{};
    passInfo.colorAttachmentInfo = {};
    passInfo.depthAttachmentInfo = depthAttachment;
    passInfo.layout = EPipelineLayout::Scene;
    passInfo.viewportWidth = config::shadowResolution;
    passInfo.viewportHeight = config::shadowResolution;
    passInfo.clearValues = fGetClearValues(clearColor, 0, clearDepth);
    passInfo.subpassIndex = 0;

    createRenderPass(ERenderPass::Shadow, EPipeline::Shadow, &passInfo);
  }
  return RE_OK;
}

void core::MRenderer::destroyRenderPasses() {
  RE_LOG(Log, "Destroying render passes.");

  for (auto& it : system.renderPasses) {
    vkDestroyRenderPass(logicalDevice.device, it.second.renderPass, nullptr);
  }
}

RRenderPass* core::MRenderer::getRenderPass(ERenderPass type) {
  if (system.renderPasses.contains(type)) {
    return &system.renderPasses.at(type);
  }

  RE_LOG(Error, "Failed to get render pass E%d, it does not exist.", type);
  return nullptr;
}

VkRenderPass& core::MRenderer::getVkRenderPass(ERenderPass type) {
  return system.renderPasses.at(type).renderPass;
}

TResult core::MRenderer::createDynamicRenderingPasses() {
  // Create environment prefiltered pass
  // Cubemaps should've been created by createImageTargets() earlier
  {
    RDynamicRenderingInfo info{};
    info.pipeline = EPipeline::EnvSkybox;
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpEnvSkybox;
    info.vertexShader = "vs_environment.spv";
    info.fragmentShader = "fs_envFilter.spv";
    info.colorViews = {{environment.pTargetCubemap->texture.view, environment.pTargetCubemap->texture.imageFormat}};
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

    createDynamicRenderPass(EDynamicRenderingPass::Environment, &info);
  }

  return RE_OK;
}

RDynamicRenderingPass* core::MRenderer::getDynamicRenderingPass(EDynamicRenderingPass type) {
  if (system.dynamicRenderingPasses.contains(type)) {
    return &system.dynamicRenderingPasses.at(type);
  }

  RE_LOG(
    Error,
    "Failed to get dynamic rendering data for pass E%d, it does not exist.",
    type);
  return nullptr;
}

TResult core::MRenderer::configureRenderPasses() {
  // render pass begin / initialize setup
  system.renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  system.renderPassBeginInfo.renderArea.offset = { 0, 0 };
  system.renderPassBeginInfo.renderArea.extent = swapchain.imageExtent;

  // will be set during the frame draw
  system.renderPassBeginInfo.renderPass = VK_NULL_HANDLE;
  system.renderPassBeginInfo.framebuffer = VK_NULL_HANDLE;

  return RE_OK;
}