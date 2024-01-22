#include "pch.h"
#include "core/core.h"
#include "core/material/texture.h"
#include "core/managers/resources.h"
#include "core/managers/renderer.h"

TResult core::MRenderer::createDynamicRenderingPass(EDynamicRenderingPass passId, RDynamicRenderingInfo* pInfo) {
  VkRenderingAttachmentInfo defaultAttachment{};
  defaultAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  defaultAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  defaultAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  defaultAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  defaultAttachment.clearValue = { 1.0f, 0.0f, 0.0f, 0.0f };

  // Create the dynamic render pass if it doesn't already exist.
  if (system.dynamicRenderingPasses.contains(passId)) {
    RE_LOG(Warning, "Attempting to create dynamic rendering pass E%d, but it already exists.", passId);
    return RE_WARNING;
  }

  RDynamicRenderingPass* pRenderPass = &system.dynamicRenderingPasses[passId];
  const uint32_t colorAttachmentCount = static_cast<uint32_t>(pInfo->colorViews.size());

  // Set dynamic rendering pass
  pRenderPass->passId = passId;
  pRenderPass->viewportId = pInfo->viewportId;
  pRenderPass->colorAttachmentCount = colorAttachmentCount;
  pRenderPass->layoutId = pInfo->pipelineLayout;
  pRenderPass->layout = getPipelineLayout(pRenderPass->layoutId);

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
  pipelineInfo.pRenderPass = pRenderPass;
  pipelineInfo.pDynamicPipelineInfo = &pipelineCreateInfo;
  pipelineInfo.vertexShader = pInfo->vertexShader;
  pipelineInfo.fragmentShader = pInfo->fragmentShader;
  pipelineInfo.blendEnable = pInfo->pipelineInfo.blendEnable;
  pipelineInfo.cullMode = pInfo->pipelineInfo.cullMode;
  pipelineInfo.primitiveTopology = pInfo->pipelineInfo.primitiveTopology;
  pipelineInfo.polygonMode = pInfo->pipelineInfo.polygonMode;

  RE_CHECK(createGraphicsPipeline(&pipelineInfo));

  RE_LOG(Log, "Created dynamic rendering pass E%d.", passId);
  return RE_OK;
}

TResult core::MRenderer::createDynamicRenderingPasses() {
  // Create environment pass
  // Cubemaps should've been created by createImageTargets() earlier
  {
    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpEnvironment;
    info.vertexShader = "vs_environment.spv";
    info.fragmentShader = "fs_envFilter.spv";
    info.colorViews = {{environment.pTargetCubemap->texture.view, environment.pTargetCubemap->texture.imageFormat}};
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

    createDynamicRenderingPass(EDynamicRenderingPass::Environment, &info);
  }

  // G-Buffer passes
  {
    // Backface culled opaque pass
    const uint32_t colorAttachmentCount = static_cast<uint32_t>(scene.pGBufferTargets.size());
    RTexture* pDepthAttachment = core::resources.getTexture(RTGT_DEPTH);

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_scene.spv";
    info.fragmentShader = "fs_gbuffer.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.pipelineInfo.blendEnable = VK_FALSE;
    info.pipelineInfo.cullMode = VK_CULL_MODE_BACK_BIT;

    info.colorViews.resize(colorAttachmentCount);

    for (uint8_t i = 0; i < colorAttachmentCount; ++i) {
      info.colorViews[i] = { scene.pGBufferTargets[i]->texture.view, scene.pGBufferTargets[i]->texture.imageFormat };
    }

    info.depthView = { pDepthAttachment->texture.view, pDepthAttachment->texture.imageFormat };

    createDynamicRenderingPass(EDynamicRenderingPass::OpaqueCullBack, &info);

    // Opaque pass without culling

    info.pipelineInfo.cullMode = VK_CULL_MODE_NONE;

    createDynamicRenderingPass(EDynamicRenderingPass::OpaqueCullNone, &info);

    // Blend pass without culling

    info.pipelineInfo.blendEnable = VK_TRUE;

    createDynamicRenderingPass(EDynamicRenderingPass::BlendCullNone, &info);

    // TODO: add mask and possibly front culled passes
  }

  // PBR pass, processes the result of the G-Buffer
  {
    RTexture* pColorAttachment = core::resources.getTexture(RTGT_GPBR);
    RTexture* pDepthAttachment = core::resources.getTexture(RTGT_DEPTH);

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_quad.spv";
    info.fragmentShader = "fs_pbr.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.colorViews = { { pColorAttachment->texture.view, pColorAttachment->texture.imageFormat } };
    info.depthView = { pDepthAttachment->texture.view, pDepthAttachment->texture.imageFormat };

    createDynamicRenderingPass(EDynamicRenderingPass::PBR, &info);
  }

  // Skybox pipeline, uses forward subpass of the deferred render pass
  {
    RTexture* pColorAttachment = core::resources.getTexture(RTGT_GPBR);
    RTexture* pDepthAttachment = core::resources.getTexture(RTGT_DEPTH);

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_skybox.spv";
    info.fragmentShader = "fs_skybox.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.colorViews = { { pColorAttachment->texture.view, pColorAttachment->texture.imageFormat } };
    info.depthView = { pDepthAttachment->texture.view, pDepthAttachment->texture.imageFormat };

    createDynamicRenderingPass(EDynamicRenderingPass::Skybox, &info);
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

void core::MRenderer::destroyDynamicRenderingPasses() {
  RE_LOG(Log, "Shutting down graphics pipeline.");
  destroyDescriptorSetLayouts();

  for (auto& renderPass : system.dynamicRenderingPasses) {
    vkDestroyPipeline(logicalDevice.device, renderPass.second.pipeline, nullptr);
  }

  for (auto& layout : system.layouts) {
    vkDestroyPipelineLayout(logicalDevice.device, layout.second, nullptr);
  }
}