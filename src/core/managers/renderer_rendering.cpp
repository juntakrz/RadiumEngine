#include "pch.h"
#include "core/core.h"
#include "core/managers/actors.h"
#include "core/managers/time.h"
#include "core/managers/world.h"
#include "core/world/actors/entity.h"
#include "core/model/model.h"
#include "core/material/texture.h"
#include "core/managers/renderer.h"

void core::MRenderer::drawBoundEntities(VkCommandBuffer commandBuffer, EDynamicRenderingPass passOverride) {
  // go through bound models and generate draw calls for each
  renderView.refresh();

  if (passOverride == EDynamicRenderingPass::Null) {
    passOverride = renderView.pCurrentPass->passId;
  }

  for (WModel* pModel : scene.pModelReferences) {
    auto& primitives = pModel->getPrimitives();

    for (const auto& primitive : primitives) {
      if (!checkPass(primitive->pInitialMaterial->passFlags, passOverride)) continue;

      renderPrimitive(commandBuffer, primitive, pModel);
    }
  }
}

void core::MRenderer::renderPrimitive(VkCommandBuffer cmdBuffer,
                                      WPrimitive* pPrimitive,
                                      WModel* pModel) {
  uint32_t instanceCount = static_cast<uint32_t>(pPrimitive->instanceData.size());
  int32_t vertexOffset = (int32_t)pModel->m_sceneVertexOffset + (int32_t)pPrimitive->vertexOffset;
  uint32_t indexOffset = pModel->m_sceneIndexOffset + pPrimitive->indexOffset;

  VkDeviceSize instanceOffset = sizeof(RInstanceData) * pPrimitive->instanceData[0].instanceIndex;
  vkCmdBindVertexBuffers(cmdBuffer, 1, 1, &scene.instanceBuffers[renderView.frameInFlight].buffer, &instanceOffset);

  // TODO: implement draw indirect
  vkCmdDrawIndexed(cmdBuffer, pPrimitive->indexCount, instanceCount, indexOffset, vertexOffset, 0);
}

void core::MRenderer::renderEnvironmentMaps(
    VkCommandBuffer commandBuffer, const uint32_t frameInterval) {
  if (renderView.framesRendered % frameInterval) return;

  // Everything is rendered and processed, the job is done
  if (environment.tracking.layer > 17) {
    environment.tracking.layer = 0;
    renderView.generateEnvironmentMaps = false;
    return;

  // Irradiance map is processed, compute prefiltered map one layer per frame
  } else if (environment.tracking.layer > 11) {
    // Cubemap face index
    environment.computeJobs.prefiltered.intValues.y = environment.tracking.layer - 12;
    // Total samples
    environment.computeJobs.prefiltered.intValues.z = 128;

    queueComputeJob(&environment.computeJobs.prefiltered);
    environment.tracking.layer++;
    return;

  // All 6 faces are rendered, compute irradiance map one layer per frame
  } else if (environment.tracking.layer > 5) {
    // Cubemap face index
    environment.computeJobs.irradiance.intValues.y = environment.tracking.layer - 6;

    queueComputeJob(&environment.computeJobs.irradiance);
    environment.tracking.layer++;
    return;
  }

  EViewport viewportIndex = EViewport::vpEnvironment;
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(EDynamicRenderingPass::EnvSkybox);
  renderView.pCurrentPass = pRenderPass;

  setCamera(RCAM_ENV);
  getCamera()->setRotation(environment.cameraTransformVectors[environment.tracking.layer]);
  updateSceneUBO(renderView.frameInFlight);

  // start rendering an appropriate camera view / layer
  environment.subresourceRange.baseArrayLayer = environment.tracking.layer;
  setImageLayout(commandBuffer, environment.pTargetCubemap, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, environment.subresourceRange);

  VkRenderingAttachmentInfo overrideAttachment = pRenderPass->renderingInfo.pColorAttachments[0];
  overrideAttachment.imageView = environment.pTargetCubemap->texture.cubemapFaceViews[environment.tracking.layer];
  overrideAttachment.imageLayout = environment.pTargetCubemap->texture.imageLayout;

  VkRenderingInfo overrideRenderingInfo = pRenderPass->renderingInfo;
  overrideRenderingInfo.pColorAttachments = &overrideAttachment;

  vkCmdBeginRendering(commandBuffer, &overrideRenderingInfo);

  setViewport(commandBuffer, viewportIndex);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->pipeline);

  const uint32_t dynamicOffset = config::scene::cameraBlockSize * view.pActiveCamera->getViewBufferIndex();

  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      renderView.pCurrentPass->layout, 0, 1,
      &scene.descriptorSets[renderView.frameInFlight], 1,
      &dynamicOffset);

  drawBoundEntities(commandBuffer);

  vkCmdEndRendering(commandBuffer);

  setImageLayout(commandBuffer, environment.pTargetCubemap, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, environment.subresourceRange);

  // increase layer count to write to the next cubemap face
  environment.tracking.layer++;
}

void core::MRenderer::executeRenderingPass(VkCommandBuffer commandBuffer, EDynamicRenderingPass passId,
                                           RMaterial* pPushMaterial, bool renderQuad) {
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(passId);
  renderView.pCurrentPass = pRenderPass;
  VkRenderingInfo* pRenderingInfo = &renderView.pCurrentPass->renderingInfo;

  // Transition images to correct layouts for rendering if required
  if (pRenderPass->validateColorAttachmentLayout) {
    VkImageSubresourceRange subRange{};
    subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subRange.baseArrayLayer = 0u;
    subRange.baseMipLevel = 0u;

    for (uint8_t i = 0; i < pRenderPass->colorAttachmentCount; ++i) {
      RTexture* pImage = pRenderPass->pImageReferences[i];
      subRange.layerCount = pImage->texture.layerCount;
      subRange.levelCount = pImage->texture.levelCount;

      setImageLayout(commandBuffer, pImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subRange);
    }
  }

  if (pRenderPass->validateDepthAttachmentLayout) {
    VkImageSubresourceRange subRange{};
    subRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    subRange.baseArrayLayer = 0u;
    subRange.baseMipLevel = 0u;
  }

  vkCmdBeginRendering(commandBuffer, pRenderingInfo);

  setViewport(commandBuffer, renderView.pCurrentPass->viewportId);
  renderView.currentViewportId = renderView.pCurrentPass->viewportId;

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->pipeline);

  const uint32_t dynamicOffset = config::scene::cameraBlockSize * view.pActiveCamera->getViewBufferIndex();

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->layout, 0,
                          1, &renderView.pCurrentSet, 1, &dynamicOffset);

  if (pPushMaterial) {
    vkCmdPushConstants(commandBuffer, renderView.pCurrentPass->layout, VK_SHADER_STAGE_FRAGMENT_BIT,
                       sizeof(RSceneVertexPCB), sizeof(RSceneFragmentPCB), &pPushMaterial->pushConstantBlock);
  }

  switch (renderQuad) {
    case true: {
      vkCmdDraw(commandBuffer, 3, 1, 0, 0);
      break;
    }
    case false: {
      drawBoundEntities(commandBuffer);
      break;
    }
  }

  vkCmdEndRendering(commandBuffer);

  // Transition image layouts after rendering to the requested layout if required
  if (pRenderPass->transitionColorAttachmentLayout) {
    VkImageSubresourceRange subRange{};
    subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subRange.baseArrayLayer = 0u;
    subRange.baseMipLevel = 0u;

    for (uint8_t j = 0; j < pRenderPass->colorAttachmentCount; ++j) {
      RTexture* pImage = pRenderPass->pImageReferences[j];
      subRange.layerCount = pImage->texture.layerCount;
      subRange.levelCount = pImage->texture.levelCount;

      setImageLayout(commandBuffer, pImage, pRenderPass->colorAttachmentsOutLayout, subRange);
    }
  }
  // TODO:
  if (pRenderPass->transitionDepthAttachmentLayout) {
    VkImageSubresourceRange subRange{};
    subRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    subRange.baseArrayLayer = 0u;
    subRange.baseMipLevel = 0u;
  }
}

void core::MRenderer::prepareFrameResources(VkCommandBuffer commandBuffer) {
  /*scene.transparencyLinkedListData.nodeCount = 0u;
  memcpy(scene.transparencyLinkedListDataBuffer.allocInfo.pMappedData, &scene.transparencyLinkedListData, sizeof(uint32_t));*/

  VkDeviceSize vbOffset = 0u;
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &scene.vertexBuffer.buffer, &vbOffset);
  vkCmdBindVertexBuffers(commandBuffer, 1, 1, &scene.instanceBuffers[renderView.frameInFlight].buffer, &vbOffset);

  vkCmdBindIndexBuffer(commandBuffer, scene.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

  uint32_t transformOffsets[3] = { 0u, 0u, 0u };
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    getPipelineLayout(EPipelineLayout::Scene), 1, 1, &scene.transformDescriptorSet, 3, transformOffsets);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    getPipelineLayout(EPipelineLayout::Scene), 2, 1, &material.descriptorSet, 0, nullptr);

  // Update transparency data
  vkCmdClearColorImage(commandBuffer, scene.pTransparencyStorageTexture->texture.image, scene.pTransparencyStorageTexture->texture.imageLayout,
    &scene.transparencyLinkedListClearColor, 1u, &scene.transparencySubRange);
  
  vkCmdFillBuffer(commandBuffer, scene.transparencyLinkedListBuffer.buffer, 0, sizeof(uint32_t), 0);
}

void core::MRenderer::executeShadowPass(VkCommandBuffer commandBuffer, const uint32_t cascadeIndex) {
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(EDynamicRenderingPass::Shadow);
  renderView.pCurrentPass = pRenderPass;

  VkRenderingAttachmentInfo overrideAttachment = *pRenderPass->renderingInfo.pDepthAttachment;
  overrideAttachment.imageView = pRenderPass->pImageReferences[0]->texture.extraViews[cascadeIndex].imageView;

  VkRenderingInfo overrideInfo{};
  overrideInfo = pRenderPass->renderingInfo;
  overrideInfo.pDepthAttachment = &overrideAttachment;

  if (cascadeIndex == 0u) {
    RTexture* pShadowTexture = pRenderPass->pImageReferences[0];

    VkImageSubresourceRange subRange{};
    subRange.aspectMask = pShadowTexture->texture.aspectMask;
    subRange.baseArrayLayer = 0u;
    subRange.layerCount = pShadowTexture->texture.layerCount;
    subRange.baseMipLevel = 0u;
    subRange.levelCount = pShadowTexture->texture.levelCount;

    setImageLayout(commandBuffer, pShadowTexture, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, subRange);
  }

  scene.vertexPushBlock.cascadeIndex = cascadeIndex;

  vkCmdBeginRendering(commandBuffer, &overrideInfo);

  setViewport(commandBuffer, renderView.pCurrentPass->viewportId);
  renderView.currentViewportId = renderView.pCurrentPass->viewportId;

  const uint32_t dynamicOffset = config::scene::cameraBlockSize * view.pActiveCamera->getViewBufferIndex();

  // Normal shadow pass
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->pipeline);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->layout, 0,
    1, &renderView.pCurrentSet, 1, &dynamicOffset);

  vkCmdPushConstants(commandBuffer, renderView.pCurrentPass->layout, VK_SHADER_STAGE_VERTEX_BIT, 0u,
                     sizeof(RSceneVertexPCB), &scene.vertexPushBlock);

  drawBoundEntities(commandBuffer);

  // Discard shadow pass
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    getDynamicRenderingPass(EDynamicRenderingPass::ShadowDiscard)->pipeline);

  drawBoundEntities(commandBuffer, EDynamicRenderingPass::ShadowDiscard);

  vkCmdEndRendering(commandBuffer);

  if (cascadeIndex == config::shadowCascades - 1) {
    RTexture* pShadowTexture = pRenderPass->pImageReferences[0];

    VkImageSubresourceRange subRange{};
    subRange.aspectMask = pShadowTexture->texture.aspectMask;
    subRange.baseArrayLayer = 0u;
    subRange.layerCount = pShadowTexture->texture.layerCount;
    subRange.baseMipLevel = 0u;
    subRange.levelCount = pShadowTexture->texture.levelCount;

    setImageLayout(commandBuffer, pShadowTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subRange);
  }
}

void core::MRenderer::executeAOBlurPass(VkCommandBuffer commandBuffer) {
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(EDynamicRenderingPass::PPBlur);
  renderView.pCurrentPass = pRenderPass;

  // AO blur uses two passes - horizontal and vertical
  for (uint8_t pass = 0; pass < 2; ++pass) {
    VkRenderingAttachmentInfo overrideAttachment = pRenderPass->renderingInfo.pColorAttachments[0];

    VkImageSubresourceRange subRange;
    subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subRange.baseArrayLayer = 0u;
    subRange.layerCount = 1u;
    subRange.baseMipLevel = 0u;
    subRange.levelCount = 1u;

    switch (pass) {
      case 0: {
        setImageLayout(commandBuffer, pRenderPass->pImageReferences[0], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subRange);
        overrideAttachment.imageView = pRenderPass->pImageReferences[0]->texture.view;

        break;
      }
      default: {
        RTexture* pAOTexture = material.pBlur->pOcclusion;
        setImageLayout(commandBuffer, pAOTexture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subRange);
        overrideAttachment.imageView = pAOTexture->texture.view;

        break;
      }
    }

    VkRenderingInfo overrideInfo = pRenderPass->renderingInfo;
    overrideInfo.pColorAttachments = &overrideAttachment;

    vkCmdBeginRendering(commandBuffer, &overrideInfo);

    setViewport(commandBuffer, pRenderPass->viewportId);
    renderView.currentViewportId = renderView.pCurrentPass->viewportId;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->pipeline);

    // Used by internal shader operations to determine which type of processing to perform
    material.pBlur->pushConstantBlock.textureSets = pass;

    vkCmdPushConstants(commandBuffer, renderView.pCurrentPass->layout, VK_SHADER_STAGE_FRAGMENT_BIT,
      sizeof(RSceneVertexPCB), sizeof(RSceneFragmentPCB), &material.pBlur->pushConstantBlock);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRendering(commandBuffer);

    switch (pass) {
    case 0: {
      setImageLayout(commandBuffer, pRenderPass->pImageReferences[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subRange);

      break;
    }
    default: {
      RTexture* pAOTexture = material.pBlur->pOcclusion;
      setImageLayout(commandBuffer, pAOTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subRange);

      break;
    }
    }
  }
}

void core::MRenderer::executePostProcessTAAPass(VkCommandBuffer commandBuffer) {
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(EDynamicRenderingPass::PPTAA);
  renderView.pCurrentPass = pRenderPass;

  RTexture* pTAATexture = pRenderPass->pImageReferences[0];

  VkImageSubresourceRange subRange{};
  subRange.aspectMask = pTAATexture->texture.aspectMask;
  subRange.baseArrayLayer = 0u;
  subRange.layerCount = 1u;
  subRange.baseMipLevel = 0u;
  subRange.levelCount = 1u;
  
  setImageLayout(commandBuffer, pTAATexture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subRange);

  vkCmdBeginRendering(commandBuffer, &pRenderPass->renderingInfo);

  setViewport(commandBuffer, pRenderPass->viewportId);
  renderView.currentViewportId = renderView.pCurrentPass->viewportId;

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->pipeline);

  vkCmdPushConstants(commandBuffer, renderView.pCurrentPass->layout, VK_SHADER_STAGE_FRAGMENT_BIT,
    sizeof(RSceneVertexPCB), sizeof(RSceneFragmentPCB), &material.pGPBR->pushConstantBlock);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  vkCmdEndRendering(commandBuffer);

  setImageLayout(commandBuffer, pTAATexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subRange);

  copyImage(commandBuffer, postprocess.pTAATexture->texture.image, postprocess.pPreviousFrameTexture->texture.image,
    postprocess.pTAATexture->texture.imageLayout, postprocess.pPreviousFrameTexture->texture.imageLayout, postprocess.previousFrameCopy);
}

void core::MRenderer::executePostProcessSamplingPass(VkCommandBuffer commandBuffer, const uint32_t imageViewIndex,
                                                     const bool upsample) {
  // Store a shader coordinate into either PBR texture or downsampling texture and its mip level
  material.pGPBR->pushConstantBlock.textureSets = imageViewIndex;
  postprocess.bloomSubRange.baseMipLevel = imageViewIndex;

  setImageLayout(commandBuffer, postprocess.pBloomTexture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, postprocess.bloomSubRange);

  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass((upsample) ? EDynamicRenderingPass::PPUpsample : EDynamicRenderingPass::PPDownsample);
  renderView.pCurrentPass = pRenderPass;

  VkRenderingAttachmentInfo overrideAttachment = pRenderPass->renderingInfo.pColorAttachments[0];
  overrideAttachment.imageView = pRenderPass->pImageReferences[0]->texture.extraViews[imageViewIndex].imageView;

  VkRenderingInfo overrideInfo = pRenderPass->renderingInfo;
  overrideInfo.pColorAttachments = &overrideAttachment;
  overrideInfo.renderArea = postprocess.scissors[imageViewIndex];

  vkCmdBeginRendering(commandBuffer, &overrideInfo);

  vkCmdSetViewport(commandBuffer, 0, 1, &postprocess.viewports[imageViewIndex]);
  vkCmdSetScissor(commandBuffer, 0, 1, &postprocess.scissors[imageViewIndex]);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pRenderPass->pipeline);

  vkCmdPushConstants(commandBuffer, pRenderPass->layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(RSceneVertexPCB),
                     sizeof(RSceneFragmentPCB), &material.pGPBR->pushConstantBlock);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  vkCmdEndRendering(commandBuffer);

  setImageLayout(commandBuffer, postprocess.pBloomTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, postprocess.bloomSubRange);
}

void core::MRenderer::executePostProcessGetExposurePass(VkCommandBuffer commandBuffer) {
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(EDynamicRenderingPass::PPGetExposure);
  renderView.pCurrentPass = pRenderPass;

  VkImageSubresourceRange subRange{};
  subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subRange.baseArrayLayer = 0u;
  subRange.layerCount = 1u;
  subRange.baseMipLevel = 0u;
  subRange.levelCount = postprocess.pExposureTexture->texture.levelCount;

  setImageLayout(commandBuffer, postprocess.pExposureTexture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subRange);

  VkRenderingInfo overrideInfo{};
  overrideInfo = pRenderPass->renderingInfo;
  overrideInfo.renderArea = postprocess.scissors[0];

  vkCmdBeginRendering(commandBuffer, &pRenderPass->renderingInfo);

  setViewport(commandBuffer, pRenderPass->viewportId);
  renderView.currentViewportId = renderView.pCurrentPass->viewportId;

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->pipeline);

  vkCmdPushConstants(commandBuffer, renderView.pCurrentPass->layout, VK_SHADER_STAGE_FRAGMENT_BIT,
    sizeof(RSceneVertexPCB), sizeof(RSceneFragmentPCB), &material.pGPBR->pushConstantBlock);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  vkCmdEndRendering(commandBuffer);

  generateMipMaps(commandBuffer, postprocess.pExposureTexture, postprocess.pExposureTexture->texture.levelCount);

  setImageLayout(commandBuffer, postprocess.pExposureTexture, VK_IMAGE_LAYOUT_GENERAL, subRange);

  VkImageSubresourceLayers subresource{};
  subresource.aspectMask = postprocess.pExposureTexture->texture.aspectMask;
  subresource.baseArrayLayer = 0u;
  subresource.layerCount = 1u;
  subresource.mipLevel = postprocess.pExposureTexture->texture.levelCount - 1;

  copyImageToBuffer(commandBuffer, postprocess.pExposureTexture, postprocess.exposureStorageBuffer.buffer, 16, 16, &subresource);
}

void core::MRenderer::executePostProcessPass(VkCommandBuffer commandBuffer) {
  const uint8_t levelCount = postprocess.pBloomTexture->texture.levelCount;
  
  executePostProcessTAAPass(commandBuffer);
  executePostProcessGetExposurePass(commandBuffer);

  for (uint8_t downsampleIndex = 0; downsampleIndex < levelCount; ++downsampleIndex) {
    executePostProcessSamplingPass(commandBuffer, downsampleIndex, false);

  }

  // Upsample from the lower mip level and write to the one above it
  // Thus the initial index is the next to last one
  for (uint8_t upsampleIndex = levelCount - 1; upsampleIndex > 0; --upsampleIndex) {
    executePostProcessSamplingPass(commandBuffer, upsampleIndex - 1, true);
  };
}

void core::MRenderer::executePresentPass(VkCommandBuffer commandBuffer) {
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(EDynamicRenderingPass::Present);
  renderView.pCurrentPass = pRenderPass;

  RTexture* pImage = swapchain.pImages[renderView.currentFrameIndex];

  VkImageSubresourceRange subRange{};
  subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subRange.baseArrayLayer = 0u;
  subRange.layerCount = 1u;
  subRange.baseMipLevel = 0u;
  subRange.levelCount = 1u;

  setImageLayout(commandBuffer, pImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subRange);

  VkRenderingAttachmentInfo overrideAttachment = pRenderPass->renderingInfo.pColorAttachments[0];
  overrideAttachment.imageView = pImage->texture.view;
  overrideAttachment.imageLayout = pImage->texture.imageLayout;

  VkRenderingInfo overrideRenderingInfo = pRenderPass->renderingInfo;
  overrideRenderingInfo.pColorAttachments = &overrideAttachment;

  vkCmdBeginRendering(commandBuffer, &overrideRenderingInfo);

  setViewport(commandBuffer, renderView.pCurrentPass->viewportId);
  renderView.currentViewportId = renderView.pCurrentPass->viewportId;

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->pipeline);

  vkCmdPushConstants(commandBuffer, renderView.pCurrentPass->layout, VK_SHADER_STAGE_FRAGMENT_BIT,
                     sizeof(RSceneVertexPCB), sizeof(RSceneFragmentPCB), &material.pGPBR->pushConstantBlock);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  vkCmdEndRendering(commandBuffer);

  setImageLayout(commandBuffer, pImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subRange);
}

void core::MRenderer::renderFrame() {
  TResult chkResult = RE_OK;

  vkWaitForFences(logicalDevice.device, 1,
    &sync.fenceInFlight[renderView.frameInFlight], VK_TRUE,
    UINT64_MAX);

  VkResult APIResult =
    vkAcquireNextImageKHR(logicalDevice.device, swapChain, UINT64_MAX,
      sync.semImgAvailable[renderView.frameInFlight],
      VK_NULL_HANDLE, &renderView.currentFrameIndex);

  if (APIResult == VK_ERROR_OUT_OF_DATE_KHR) {
    RE_LOG(Warning, "Out of date swap chain image. Recreating swap chain.");

    recreateSwapChain();
    return;
  }

  if (APIResult != VK_SUCCESS && APIResult != VK_SUBOPTIMAL_KHR) {
    RE_LOG(Error, "Failed to acquire valid swap chain image.");

    return;
  }

  // get new delta time between frames
  core::time.tickTimer();

  // reset fences if we will do any work this frame e.g. no swap chain
  // recreation
  vkResetFences(logicalDevice.device, 1,
    &sync.fenceInFlight[renderView.frameInFlight]);

  vkResetCommandBuffer(command.buffersGraphics[renderView.frameInFlight], NULL);

  executeQueuedComputeJobs();

  VkCommandBuffer cmdBuffer = command.buffersGraphics[renderView.frameInFlight];

  // Update lighting UBO if required
  updateLightingUBO(renderView.frameInFlight);

  // Use this frame's scene descriptor set
  renderView.pCurrentSet = scene.descriptorSets[renderView.frameInFlight];

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = nullptr;
  beginInfo.flags = 0;

  // Start recording vulkan command buffer
  if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
    RE_LOG(Error, "Failure when trying to record command buffer.");
    return;
  }

  // Prepare and bind per frame resources
  prepareFrameResources(cmdBuffer);

  /* 1. Environment generation */

  if (renderView.generateEnvironmentMaps) {
    renderEnvironmentMaps(cmdBuffer, environment.genInterval);
  }

  /* 2. Cascaded shadows */

  setCamera(view.pSunCamera);
  updateSceneUBO(renderView.frameInFlight);

  for (uint8_t cascadeIndex = 0; cascadeIndex < config::shadowCascades; ++cascadeIndex) {
    executeShadowPass(cmdBuffer, cascadeIndex);
  }

  /* 3. Main scene */

  setCamera(RCAM_MAIN);
  updateSceneUBO(renderView.frameInFlight);

  // G-Buffer passes
  executeRenderingPass(cmdBuffer, EDynamicRenderingPass::OpaqueCullBack);
  executeRenderingPass(cmdBuffer, EDynamicRenderingPass::OpaqueCullNone);
  executeRenderingPass(cmdBuffer, EDynamicRenderingPass::DiscardCullNone);
  executeRenderingPass(cmdBuffer, EDynamicRenderingPass::BlendCullNone);

  // Deferred rendering pass using G-Buffer collected data
  executeRenderingPass(cmdBuffer, EDynamicRenderingPass::PBR, material.pGBuffer, true);

  // Additional front rendering passes
  executeRenderingPass(cmdBuffer, EDynamicRenderingPass::Skybox);

  executeAOBlurPass(cmdBuffer);

  executeRenderingPass(cmdBuffer, EDynamicRenderingPass::AlphaCompositing, material.pGPBR, true);

  /* 4. Postprocessing pass */

  // Includes exposure, bloom and TAA passes
  executePostProcessPass(cmdBuffer);

  /* 5. Final presentation pass */

  executePresentPass(cmdBuffer);

  // End writing commands and prepare to submit buffer to rendering queue
  if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to end writing to command buffer.");
  }

  // Wait until image to write color data to is acquired
  VkSemaphore waitSems[] = {sync.semImgAvailable[renderView.frameInFlight]};
  VkSemaphore signalSems[] = {sync.semRenderFinished[renderView.frameInFlight]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSems;
  submitInfo.pWaitDstStageMask = waitStages;  // Each stage index corresponds to provided semaphore index
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command.buffersGraphics[renderView.frameInFlight];  // Submit command buffer recorded previously
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSems;  // Signal these after rendering is finished

  // Submit an array featuring command buffers to graphics queue and signal
  // Fence for CPU to wait for execution
  if (vkQueueSubmit(logicalDevice.queues.graphics, 1, &submitInfo,
                    sync.fenceInFlight[renderView.frameInFlight]) !=
      VK_SUCCESS) {
    RE_LOG(Error, "Failed to submit data to graphics queue.");
  }

  VkSwapchainKHR swapChains[] = {swapChain};

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSems;                   // Present when 'render finished' semaphores are signaled
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &renderView.currentFrameIndex;  // Use selected images for set swapchains
  presentInfo.pResults = nullptr;                             // For future use with more swapchains

  APIResult = vkQueuePresentKHR(logicalDevice.queues.present, &presentInfo);

  if (APIResult == VK_ERROR_OUT_OF_DATE_KHR || APIResult == VK_SUBOPTIMAL_KHR ||
      framebufferResized) {
    RE_LOG(Warning, "Recreating swap chain, reason: %d", APIResult);
    framebufferResized = false;
    recreateSwapChain();

    return;
  }

  if (APIResult != VK_SUCCESS) {
    RE_LOG(Error, "Failed to present new frame.");

    return;
  }

  // Synchronize CPU threads
  sync.asyncUpdateEntities.update();
  sync.asyncUpdateInstanceBuffers.update();

  renderView.frameInFlight = ++renderView.frameInFlight % MAX_FRAMES_IN_FLIGHT;
  ++renderView.framesRendered;
}

void core::MRenderer::renderInitFrame() {
  // Generate BRDF LUT during the initial frame
  queueComputeJob(&environment.computeJobs.LUT);

  RTexture* pNoiseTexture = core::resources.getTexture(RTGT_NOISEMAP);
  core::resources.writeTexture(pNoiseTexture, system.randomOffsets.data(),
    sizeof(glm::vec4) * system.randomOffsets.size());

  renderFrame();
}