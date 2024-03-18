#include "pch.h"
#include "core/core.h"
#include "core/material/texture.h"
#include "core/managers/resources.h"
#include "core/managers/renderer.h"

TResult core::MRenderer::createDynamicRenderingPass(EDynamicRenderingPass passId, RDynamicRenderingInfo* pInfo) {
  VkRenderingAttachmentInfo defaultAttachment{};
  defaultAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  defaultAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  defaultAttachment.loadOp = (pInfo->clearColorAttachments) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
  defaultAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  defaultAttachment.clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};
  defaultAttachment.clearValue.depthStencil = {1.0f, 0u};

  VkRenderingAttachmentInfo defaultDepthStencilAttachment = defaultAttachment;
  defaultDepthStencilAttachment.loadOp = (pInfo->clearDepthAttachment) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;


  // Create the dynamic render pass if it doesn't already exist.
  if (system.dynamicRenderingPasses.contains(passId)) {
    RE_LOG(Warning, "Attempting to create dynamic rendering pass E%d, but it already exists.", passId);
    return RE_WARNING;
  }

  RDynamicRenderingPass* pRenderPass = &system.dynamicRenderingPasses[passId];
  const uint32_t colorAttachmentCount = static_cast<uint32_t>(pInfo->colorAttachments.size());

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

  // Store references to original images
  pRenderPass->pImageReferences = std::vector<RTexture*>(colorAttachmentCount + 2u, nullptr);

  if (colorAttachmentCount) {
    pRenderPass->colorAttachments.resize(colorAttachmentCount);
    
    for (uint8_t attachmentIndex = 0; attachmentIndex < colorAttachmentCount; ++attachmentIndex) {
      pRenderPass->colorAttachments[attachmentIndex] = defaultAttachment;
      pRenderPass->colorAttachments[attachmentIndex].imageView = pInfo->colorAttachments[attachmentIndex].view;
      pRenderPass->colorAttachments[attachmentIndex].clearValue = pInfo->colorAttachmentClearValue;

      pRenderPass->pImageReferences[attachmentIndex] = pInfo->colorAttachments[attachmentIndex].pImage;
    }

    pRenderPass->renderingInfo.pColorAttachments = pRenderPass->colorAttachments.data();
    pRenderPass->renderingInfo.colorAttachmentCount = (pInfo->singleColorAttachmentAtRuntime) ? 1u : colorAttachmentCount;
  }

  if (pInfo->depthAttachment.view) {
    pRenderPass->depthAttachment = defaultDepthStencilAttachment;
    pRenderPass->depthAttachment.imageView = pInfo->depthAttachment.view;
    pRenderPass->depthAttachment.imageLayout = pInfo->depthLayout;

    pRenderPass->renderingInfo.pDepthAttachment = &pRenderPass->depthAttachment;

    // Store reference to the depth image
    pRenderPass->pImageReferences[colorAttachmentCount] = pInfo->depthAttachment.pImage;
  }

  if (pInfo->stencilAttachment.view) {
    pRenderPass->stencilAttachment = defaultDepthStencilAttachment;
    pRenderPass->stencilAttachment.imageView = pInfo->stencilAttachment.view;
    pRenderPass->stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;

    pRenderPass->renderingInfo.pStencilAttachment = &pRenderPass->stencilAttachment;

    // Store reference to the stencil image
    pRenderPass->pImageReferences[colorAttachmentCount + 1u] = pInfo->stencilAttachment.pImage;
  }

  // Layout transition settings
  pRenderPass->validateColorAttachmentLayout = pInfo->layoutInfo.validateColorAttachmentLayout;
  pRenderPass->validateDepthAttachmentLayout = pInfo->layoutInfo.validateDepthAttachmentLayout;
  pRenderPass->transitionColorAttachmentLayout = pInfo->layoutInfo.transitionColorAttachmentLayout;
  pRenderPass->transitionDepthAttachmentLayout = pInfo->layoutInfo.transitionDepthAttachmentLayout;
  pRenderPass->colorAttachmentsOutLayout = pInfo->layoutInfo.colorAttachmentsOutLayout;

  // Pipeline creation
  std::vector<VkFormat> colorAttachmentFormats(colorAttachmentCount);

  for (uint32_t j = 0; j < colorAttachmentCount; ++j) {
    colorAttachmentFormats[j] = pInfo->colorAttachments[j].format;
  }

  VkPipelineRenderingCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  pipelineCreateInfo.viewMask = 0;

  if (colorAttachmentCount) {
    pipelineCreateInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
    pipelineCreateInfo.colorAttachmentCount = (pInfo->singleColorAttachmentAtRuntime) ? 1u : colorAttachmentCount;
  }

  if (pInfo->depthAttachment.view) {
    pipelineCreateInfo.depthAttachmentFormat = pInfo->depthAttachment.format;
  }

  if (pInfo->stencilAttachment.view) {
    pipelineCreateInfo.stencilAttachmentFormat = pInfo->stencilAttachment.format;
  }

  RGraphicsPipelineInfo pipelineInfo{};
  pipelineInfo.pRenderPass = pRenderPass;
  pipelineInfo.pDynamicPipelineInfo = &pipelineCreateInfo;
  pipelineInfo.vertexShader = pInfo->vertexShader;
  pipelineInfo.geometryShader = pInfo->geometryShader;
  pipelineInfo.fragmentShader = pInfo->fragmentShader;
  pipelineInfo.enableBlending = pInfo->pipelineInfo.enableBlending;
  pipelineInfo.enableDepthWrite = pInfo->pipelineInfo.enableDepthWrite;
  pipelineInfo.enableDepthTest = pInfo->pipelineInfo.enableDepthTest;
  pipelineInfo.cullMode = pInfo->pipelineInfo.cullMode;
  pipelineInfo.primitiveTopology = pInfo->pipelineInfo.primitiveTopology;
  pipelineInfo.polygonMode = pInfo->pipelineInfo.polygonMode;

  // Hacky way of copying depth bias settings without defining a whole new struct
  memcpy(&pipelineInfo.depthBias, &pInfo->pipelineInfo.depthBias, sizeof(pInfo->pipelineInfo.depthBias));

  RE_CHECK(createGraphicsPipeline(&pipelineInfo));

  RE_LOG(Log, "Created dynamic rendering pass E%d.", passId);
  return RE_OK;
}

TResult core::MRenderer::createDynamicRenderingPasses() {
  // Create environment pass
  // Cubemaps should've been created by createImageTargets() earlier
  {
    RTexture* pDepthAttachment = scene.pDepthTargets[0];

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpEnvironment;
    info.vertexShader = "vs_skybox.spv";
    info.fragmentShader = "fs_skybox.spv";
    info.colorAttachments =
         {{environment.pTargetCubemap, environment.pTargetCubemap->texture.view, environment.pTargetCubemap->texture.imageFormat}};

    // TODO: use a unique depth attachment for the environmental pass to avoid potential issues with the main per frame depth buffering
    info.depthAttachment = { pDepthAttachment, pDepthAttachment->texture.view, pDepthAttachment->texture.imageFormat };

    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

    info.clearDepthAttachment = true;
    info.layoutInfo.validateColorAttachmentLayout = true;
    info.layoutInfo.transitionColorAttachmentLayout = true;

    createDynamicRenderingPass(EDynamicRenderingPass::EnvSkybox, &info);
  }

  // Shadow pass
  {
    // Main shadow pass (vertex shader only)
    RTexture* pDepthAttachment = core::resources.getTexture(RTGT_SHADOW);

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpShadow;
    info.vertexShader = "vs_shadowPass.spv";
    //info.geometryShader = "gs_shadowPass.spv";    // Seems to be slower than using separate depth passes
    info.pipelineInfo.enableBlending = VK_FALSE;
    info.pipelineInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    info.pipelineInfo.depthBias.enable = VK_TRUE;
    info.clearDepthAttachment = true;
    info.depthAttachment = { pDepthAttachment, pDepthAttachment->texture.view, pDepthAttachment->texture.imageFormat };

    createDynamicRenderingPass(EDynamicRenderingPass::Shadow, &info);

    // Discard shadow pass
    info.fragmentShader = "fs_shadowPass.spv";
    info.pipelineInfo.cullMode = VK_CULL_MODE_NONE;

    createDynamicRenderingPass(EDynamicRenderingPass::ShadowDiscard, &info);
  }

  // G-Buffer passes
  {
    // Backface culled opaque pass
    uint32_t colorAttachmentCount = static_cast<uint32_t>(scene.pGBufferTargets.size());
    RTexture* pDepthAttachment = scene.pDepthTargets[0];

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_scene.spv";
    info.fragmentShader = "fs_gbuffer.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.clearColorAttachments = true;
    info.clearDepthAttachment = true;
    info.pipelineInfo.enableBlending = VK_FALSE;
    info.pipelineInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    info.layoutInfo.validateColorAttachmentLayout = true;

    info.colorAttachments.resize(colorAttachmentCount);

    for (uint8_t i = 0; i < colorAttachmentCount; ++i) {
      info.colorAttachments[i] =
           { scene.pGBufferTargets[i], scene.pGBufferTargets[i]->texture.view, scene.pGBufferTargets[i]->texture.imageFormat };
    }

    info.depthAttachment = { pDepthAttachment, pDepthAttachment->texture.view, pDepthAttachment->texture.imageFormat };

    createDynamicRenderingPass(EDynamicRenderingPass::OpaqueCullBack, &info);

    // Opaque pass without culling

    info.clearColorAttachments = false;
    info.clearDepthAttachment = false;
    info.pipelineInfo.cullMode = VK_CULL_MODE_NONE;

    createDynamicRenderingPass(EDynamicRenderingPass::OpaqueCullNone, &info);

    // Discard pass without culling
    info.fragmentShader = "fs_gbufferDiscard.spv";

    createDynamicRenderingPass(EDynamicRenderingPass::DiscardCullNone, &info);

    // Blend pass without culling and depth writes

    //colorAttachmentCount = static_cast<uint32_t>(scene.pABufferTargets.size());

    info.fragmentShader = "fs_abuffer.spv";
    info.pipelineInfo.enableBlending = VK_TRUE;
    info.pipelineInfo.enableDepthWrite = VK_FALSE;
    info.layoutInfo.transitionColorAttachmentLayout = true;
    info.clearColorAttachments = true;
    info.colorAttachments.resize(0);

    createDynamicRenderingPass(EDynamicRenderingPass::BlendCullNone, &info);

    // TODO: add mask and possibly front culled passes
  }

  // PBR pass, processes the result of the G-Buffer
  {
    std::vector<RTexture*> pColorAttachments = { core::resources.getTexture(RTGT_GPBR), core::resources.getTexture(RTGT_PPAO) };

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_quad.spv";
    info.fragmentShader = "fs_pbr.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.clearColorAttachments = true;
    info.layoutInfo.validateColorAttachmentLayout = true;

    info.colorAttachments.resize(pColorAttachments.size());
    for (uint8_t i = 0; i < pColorAttachments.size(); ++i) {
      info.colorAttachments[i] = { pColorAttachments[i], pColorAttachments[i]->texture.view, pColorAttachments[i]->texture.imageFormat};
    }

    createDynamicRenderingPass(EDynamicRenderingPass::PBR, &info);
  }

  // Skybox pass, front rendering addition to deferred passes
  {
    RTexture* pColorAttachment = core::resources.getTexture(RTGT_GPBR);
    RTexture* pDepthAttachment = scene.pDepthTargets[0];

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_skybox.spv";
    info.fragmentShader = "fs_skybox.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.pipelineInfo.enableDepthWrite = VK_FALSE;
    info.layoutInfo.transitionColorAttachmentLayout = true;
    info.colorAttachments = {{ pColorAttachment, pColorAttachment->texture.view, pColorAttachment->texture.imageFormat }};
    info.depthAttachment = { pDepthAttachment, pDepthAttachment->texture.view, pDepthAttachment->texture.imageFormat };

    createDynamicRenderingPass(EDynamicRenderingPass::Skybox, &info);
  }

  // Alpha compositing pass, return the results of alpha buffer
  {
    RTexture* pColorAttachment = core::resources.getTexture(RTGT_APBR);

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_quad.spv";
    info.fragmentShader = "fs_transparencyLayer.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.layoutInfo.validateColorAttachmentLayout = true;
    info.layoutInfo.transitionColorAttachmentLayout = true;
    info.colorAttachments = {{ pColorAttachment, pColorAttachment->texture.view, pColorAttachment->texture.imageFormat }};

    createDynamicRenderingPass(EDynamicRenderingPass::AlphaCompositing, &info);
  }

  // "Post processing" blur pass
  {
    RTexture* pColorAttachment = core::resources.getTexture(RTGT_PPBLUR);

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_quad.spv";
    info.fragmentShader = "fs_ppBlur.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.layoutInfo.transitionColorAttachmentLayout = true;
    info.colorAttachments = {{ pColorAttachment, pColorAttachment->texture.view, pColorAttachment->texture.imageFormat }};

    createDynamicRenderingPass(EDynamicRenderingPass::PPBlur, &info);
  }

  // "Post processing downsample" pass
  {
    RTexture* pColorAttachment = core::resources.getTexture(RTGT_PPBLOOM);

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_quad.spv";
    info.fragmentShader = "fs_ppDownsample.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.layoutInfo.validateColorAttachmentLayout = true;
    info.layoutInfo.transitionColorAttachmentLayout = true;
    info.layoutInfo.colorAttachmentsOutLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    info.colorAttachments = {{ pColorAttachment, pColorAttachment->texture.view, pColorAttachment->texture.imageFormat }};

    createDynamicRenderingPass(EDynamicRenderingPass::PPDownsample, &info);
  }

  // "Post processing upsample" pass
  {
    RTexture* pColorAttachment = core::resources.getTexture(RTGT_PPBLOOM);

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_quad.spv";
    info.fragmentShader = "fs_ppUpsample.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.layoutInfo.validateColorAttachmentLayout = true;
    info.layoutInfo.transitionColorAttachmentLayout = true;
    info.layoutInfo.colorAttachmentsOutLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    info.colorAttachments = {{ pColorAttachment, pColorAttachment->texture.view, pColorAttachment->texture.imageFormat }};

    createDynamicRenderingPass(EDynamicRenderingPass::PPUpsample, &info);
  }

  // "Post processing get exposure" pass
  {
    RTexture* pColorAttachment = core::resources.getTexture(RTGT_EXPOSUREMAP);

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpEnvIrrad;          // Reuse irradiance resolution for exposure map
    info.vertexShader = "vs_quad.spv";
    info.fragmentShader = "fs_ppGetExposure.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.layoutInfo.validateColorAttachmentLayout = true;
    info.layoutInfo.transitionColorAttachmentLayout = true;
    info.layoutInfo.colorAttachmentsOutLayout = VK_IMAGE_LAYOUT_GENERAL;
    info.colorAttachments = { { pColorAttachment, pColorAttachment->texture.view, pColorAttachment->texture.imageFormat } };

    createDynamicRenderingPass(EDynamicRenderingPass::PPGetExposure, &info);
  }

  // TAA pass, takes a PBR output up to this point and adds history and velocity data
  {
    RTexture* pColorAttachment = core::resources.getTexture(RTGT_PPTAA);

    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_quad.spv";
    info.fragmentShader = "fs_ppTAA.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.layoutInfo.transitionColorAttachmentLayout = true;
    info.colorAttachments = { { pColorAttachment, pColorAttachment->texture.view, pColorAttachment->texture.imageFormat } };

    createDynamicRenderingPass(EDynamicRenderingPass::PPTAA, &info);
  }

  // "Present" final output pass
  {
    RDynamicRenderingInfo info{};
    info.pipelineLayout = EPipelineLayout::Scene;
    info.viewportId = EViewport::vpMain;
    info.vertexShader = "vs_quad.spv";
    info.fragmentShader = "fs_present.spv";
    info.colorAttachmentClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    info.layoutInfo.validateColorAttachmentLayout = true;
    info.layoutInfo.transitionColorAttachmentLayout = true;
    info.layoutInfo.colorAttachmentsOutLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    info.singleColorAttachmentAtRuntime = true;

    info.colorAttachments.resize(swapchain.imageCount);

    for (uint8_t i = 0; i < swapchain.imageCount; ++i) {
      info.colorAttachments[i] =
      { swapchain.pImages[i], swapchain.pImages[i]->texture.view, swapchain.pImages[i]->texture.imageFormat };
    }

    createDynamicRenderingPass(EDynamicRenderingPass::Present, &info);
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