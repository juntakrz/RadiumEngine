#include "pch.h"
#include "core/core.h"
#include "core/managers/actors.h"
#include "core/managers/time.h"
#include "core/managers/world.h"
#include "core/world/actors/entity.h"
#include "core/model/model.h"
#include "core/material/texture.h"
#include "core/managers/renderer.h"

void core::MRenderer::drawBoundEntitiesIndirect(VkCommandBuffer commandBuffer, EDynamicRenderingPass passOverride) {
  // go through bound models and generate draw calls for each
  renderView.refresh();
  command.indirectCommands.clear();

  if (passOverride == EDynamicRenderingPass::Null) {
    passOverride = renderView.pCurrentPass->passId;
  }

  const uint32_t bufferIndex = renderView.frameInFlight;

  int32_t passId = helper::indirectPassFlagToIndex(passOverride);

  if (passId == -1) return;

  VkDeviceSize drawOffset = static_cast<VkDeviceSize>(scene.drawOffsets[bufferIndex][passId] * sizeof(VkDrawIndexedIndirectCommand));

  vkCmdDrawIndexedIndirect(commandBuffer, scene.drawIndirectBuffers[bufferIndex].buffer,
    drawOffset, scene.drawCounts[bufferIndex][passId], sizeof(VkDrawIndexedIndirectCommand));
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
    compute.environmentJobInfo.prefiltered.pushBlock.intValues.y = environment.tracking.layer - 12;
    // Total samples
    compute.environmentJobInfo.prefiltered.pushBlock.intValues.z = 128;

    queueComputeJob(&compute.environmentJobInfo.prefiltered);
    environment.tracking.layer++;
    return;

  // All 6 faces are rendered, compute irradiance map one layer per frame
  } else if (environment.tracking.layer > 5) {
    // Cubemap face index
    compute.environmentJobInfo.irradiance.pushBlock.intValues.y = environment.tracking.layer - 6;

    queueComputeJob(&compute.environmentJobInfo.irradiance);
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

  VkImageLayout targetLayout = (system.enableLayoutTransitions) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
  setImageLayout(commandBuffer, environment.pTargetCubemap, targetLayout, environment.subresourceRange);

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

  drawBoundEntitiesIndirect(commandBuffer);

  vkCmdEndRendering(commandBuffer);

  if (system.enableLayoutTransitions) {
    setImageLayout(commandBuffer, environment.pTargetCubemap, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, environment.subresourceRange);
  }

  // increase layer count to write to the next cubemap face
  environment.tracking.layer++;
}

void core::MRenderer::prepareFrameResources(VkCommandBuffer commandBuffer) {
  command.indirectCommandOffset = 0u;

  // Wait for instance data thread to finish its work before proceeding with the new frame
  {
    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    sync.cvInstanceDataReady.wait(lock, [this]() { return sync.isInstanceDataReady; });
    sync.isInstanceDataReady = false;

    const uint32_t bufferIndex = (renderView.frameInFlight + 1) % MAX_FRAMES_IN_FLIGHT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    if (vkGetFenceStatus(logicalDevice.device, sync.fenceUpdateBuffers[bufferIndex]) != VK_SUCCESS) {
      vkQueueSubmit(logicalDevice.queues.graphics, 1, &submitInfo, sync.fenceUpdateBuffers[bufferIndex]);
    }
  }

  VkDeviceSize vertexBufferOffsets[] = { 0u, 0u };
  VkBuffer vertexBuffers[] = { scene.vertexBuffer.buffer, scene.instanceDataBuffers[renderView.frameInFlight].buffer };

  vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, vertexBufferOffsets);
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

void core::MRenderer::prepareFrameComputeJobs() {
  const uint8_t previousFrameInFlight = (renderView.frameInFlight + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT;
  const uint8_t nextFrameInFlight = (renderView.frameInFlight + 1) % MAX_FRAMES_IN_FLIGHT;

  // Mip map previous frame's depth
  RComputeJobInfo& depthMipmappingJob = compute.imageJobInfo.mipmapping;
  depthMipmappingJob.width = config::renderWidth / 16;
  depthMipmappingJob.height = config::renderHeight / 16;

  // Samplers and images have their indexes separate in the shader, each starting with 0, so both are at [0] in this case
  depthMipmappingJob.pSamplerAttachments = {scene.pDepthTargets[previousFrameInFlight], scene.pPreviousDepthTargets[previousFrameInFlight]};
  depthMipmappingJob.pImageAttachments = {scene.pPreviousDepthTargets[previousFrameInFlight]};

  // Number of target mip maps
  depthMipmappingJob.pushBlock.intValues.x = scene.pPreviousDepthTargets[previousFrameInFlight]->texture.levelCount;

  // Type of mipmapping: from D32 to R32
  depthMipmappingJob.pushBlock.intValues.y = 0;

  RComputeJobInfo& cullingJob = compute.sceneJobInfo.culling;
  cullingJob.width = static_cast<uint32_t>(scene.totalInstances) / 32 + 1;
  cullingJob.pSamplerAttachments = {scene.pPreviousDepthTargets[nextFrameInFlight]};
  cullingJob.pBufferAttachments[0] = &scene.sceneBuffers[previousFrameInFlight];
  cullingJob.pBufferAttachments[4] = &scene.drawCountBuffers[previousFrameInFlight];
  cullingJob.pushBlock.intValues.x = static_cast<int32_t>(scene.totalInstances);
  cullingJob.pushBlock.intValues.y = static_cast<int32_t>(scene.nextPrimitiveUID);
  cullingJob.pushBlock.intValues.z = static_cast<int32_t>(view.pPrimaryCamera->getViewBufferIndex()); // index for retrieving camera matrices
  cullingJob.pushBlock.intValues.w = static_cast<int32_t>(scene.pPreviousDepthTargets[previousFrameInFlight]->texture.levelCount - 1);

  queueComputeJob(&depthMipmappingJob);
  queueComputeJob(&cullingJob);
}

void core::MRenderer::executeRenderingPass(VkCommandBuffer commandBuffer, EDynamicRenderingPass passId,
                                           RMaterial* pPushMaterial, bool renderQuad) {
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(passId);
  renderView.pCurrentPass = pRenderPass;

  VkRenderingInfo overrideInfo = renderView.pCurrentPass->renderingInfo;
  VkRenderingAttachmentInfo overrideDepthAttachment;

  if (overrideInfo.pDepthAttachment) {
    overrideDepthAttachment = *overrideInfo.pDepthAttachment;
    overrideDepthAttachment.imageView = scene.pDepthTargets[renderView.frameInFlight]->texture.view;

    overrideInfo.pDepthAttachment = &overrideDepthAttachment;
  }

  // Transition images to correct layouts for rendering if required
  VkImageLayout targetLayout = (system.enableLayoutTransitions)
    ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

  if (pRenderPass->validateColorAttachmentLayout) {
    VkImageSubresourceRange subRange{};
    subRange.baseArrayLayer = 0u;
    subRange.baseMipLevel = 0u;

    for (uint8_t i = 0; i < pRenderPass->colorAttachmentCount; ++i) {
      RTexture* pImage = pRenderPass->pImageReferences[i];
      subRange.aspectMask = pImage->texture.aspectMask;
      subRange.layerCount = pImage->texture.layerCount;
      subRange.levelCount = pImage->texture.levelCount;

      // Some pipeline barriers are required before transparency stages regardless if layouts are being transitioned
      setImageLayout(commandBuffer, pImage, targetLayout, subRange,
        (!system.enableLayoutTransitions && passId & EDynamicRenderingPass::DiscardCullNone));
    }
  }

  vkCmdBeginRendering(commandBuffer, &overrideInfo);

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
      drawBoundEntitiesIndirect(commandBuffer);
      break;
    }
  }

  vkCmdEndRendering(commandBuffer);

  // Transition image layouts after rendering to the requested layout if required
  if (pRenderPass->transitionColorAttachmentLayout && system.enableLayoutTransitions) {
    VkImageSubresourceRange subRange{};
    subRange.baseArrayLayer = 0u;
    subRange.baseMipLevel = 0u;

    for (uint8_t j = 0; j < pRenderPass->colorAttachmentCount; ++j) {
      RTexture* pImage = pRenderPass->pImageReferences[j];
      subRange.aspectMask = pImage->texture.aspectMask;
      subRange.layerCount = pImage->texture.layerCount;
      subRange.levelCount = pImage->texture.levelCount;

      setImageLayout(commandBuffer, pImage, pRenderPass->colorAttachmentsOutLayout, subRange);
    }
  }
}

void core::MRenderer::executeShadowPass(VkCommandBuffer commandBuffer, const uint32_t cascadeIndex) {
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(EDynamicRenderingPass::Shadow);
  renderView.pCurrentPass = pRenderPass;

  VkRenderingAttachmentInfo overrideAttachment = *pRenderPass->renderingInfo.pDepthAttachment;
  overrideAttachment.imageView = pRenderPass->pImageReferences[0]->texture.extraViews[cascadeIndex].imageView;

  VkRenderingInfo overrideInfo{};
  overrideInfo = pRenderPass->renderingInfo;
  overrideInfo.pDepthAttachment = &overrideAttachment;

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

  drawBoundEntitiesIndirect(commandBuffer);

  // Discard shadow pass
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    getDynamicRenderingPass(EDynamicRenderingPass::ShadowDiscard)->pipeline);

  drawBoundEntitiesIndirect(commandBuffer, EDynamicRenderingPass::ShadowDiscard);

  vkCmdEndRendering(commandBuffer);
}

void core::MRenderer::executeAOBlurPass(VkCommandBuffer commandBuffer) {
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(EDynamicRenderingPass::PPBlur);
  renderView.pCurrentPass = pRenderPass;

  // AO blur uses two passes - horizontal and vertical
  for (uint8_t pass = 0; pass < 2; ++pass) {
    VkRenderingAttachmentInfo overrideAttachment = pRenderPass->renderingInfo.pColorAttachments[0];

    switch (pass) {
      case 0: {
        overrideAttachment.imageView = pRenderPass->pImageReferences[0]->texture.view;

        break;
      }
      default: {
        RTexture* pAOTexture = material.pBlur->pOcclusion;
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
  
  VkImageLayout targetLayout = (system.enableLayoutTransitions)
    ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
  setImageLayout(commandBuffer, pTAATexture, targetLayout, subRange);

  vkCmdBeginRendering(commandBuffer, &pRenderPass->renderingInfo);

  setViewport(commandBuffer, pRenderPass->viewportId);
  renderView.currentViewportId = renderView.pCurrentPass->viewportId;

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->pipeline);

  vkCmdPushConstants(commandBuffer, renderView.pCurrentPass->layout, VK_SHADER_STAGE_FRAGMENT_BIT,
    sizeof(RSceneVertexPCB), sizeof(RSceneFragmentPCB), &material.pGPBR->pushConstantBlock);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  vkCmdEndRendering(commandBuffer);

  if (system.enableLayoutTransitions) {
    setImageLayout(commandBuffer, pTAATexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subRange);
  }

  copyImage(commandBuffer, postprocess.pTAATexture, postprocess.pPreviousFrameTexture,
            postprocess.previousFrameCopy);
}

void core::MRenderer::executePostProcessSamplingPass(VkCommandBuffer commandBuffer, const uint32_t imageViewIndex,
                                                     const bool upsample) {
  // Store a mip level index into either PBR texture or downsampling texture sets
  material.pGPBR->pushConstantBlock.textureSets = imageViewIndex;
  postprocess.bloomSubRange.baseMipLevel = imageViewIndex;

  VkImageLayout targetLayout = (system.enableLayoutTransitions)
    ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
  setImageLayout(commandBuffer, postprocess.pBloomTexture, targetLayout, postprocess.bloomSubRange);

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

  // Synchronization pipeline barrier is still required for every mip level of the bloom pass
  (system.enableLayoutTransitions)
    ? setImageLayout(commandBuffer, postprocess.pBloomTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, postprocess.bloomSubRange)
    : setImageLayout(commandBuffer, postprocess.pBloomTexture, VK_IMAGE_LAYOUT_GENERAL, postprocess.bloomSubRange, true);
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

  if (system.enableLayoutTransitions) {
    setImageLayout(commandBuffer, postprocess.pExposureTexture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subRange);
  }

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

  const VkFence fences[] = { sync.fenceInFlight[renderView.frameInFlight], sync.fenceUpdateBuffers[renderView.frameInFlight] };
  const uint32_t fenceCount = 2;
  vkWaitForFences(logicalDevice.device, fenceCount, fences, VK_TRUE, UINT64_MAX);

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

  // Get new delta time between frames
  core::time.tickTimer();

  // Reset fences if we will do any work this frame e.g. no swap chain recreation
  vkResetFences(logicalDevice.device, fenceCount, fences);

  vkResetCommandBuffer(command.buffersGraphics[renderView.frameInFlight], NULL);

  prepareFrameComputeJobs();
  executeQueuedComputeJobs(command.buffersCompute[renderView.frameInFlight]);

  // Update lighting UBO if required
  updateLightingUBO(renderView.frameInFlight);

  //std::this_thread::sleep_for(std::chrono::milliseconds(30));

  // Use this frame's scene descriptor set
  renderView.pCurrentSet = scene.descriptorSets[renderView.frameInFlight];

  VkCommandBuffer commandBuffer = command.buffersGraphics[renderView.frameInFlight];

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = nullptr;
  beginInfo.flags = 0;

  // Start recording vulkan command buffer
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    RE_LOG(Error, "Failure when trying to record command buffer.");
    return;
  }

  // Prepare and bind per frame resources
  prepareFrameResources(commandBuffer);

  /* 1. Environment generation */
  if (renderView.generateEnvironmentMaps && renderView.framesRendered > MAX_FRAMES_IN_FLIGHT) {
    renderEnvironmentMaps(commandBuffer, environment.genInterval);
  }

  /* 2. Cascaded shadows */

  setCamera(view.pSunCamera);
  updateSceneUBO(renderView.frameInFlight);

  for (uint8_t cascadeIndex = 0; cascadeIndex < config::shadowCascades; ++cascadeIndex) {
    executeShadowPass(commandBuffer, cascadeIndex);
  }

  /* 3. Main scene */
  setCamera(view.pPrimaryCamera);
  updateSceneUBO(renderView.frameInFlight);

  // G-Buffer passes
  executeRenderingPass(commandBuffer, EDynamicRenderingPass::OpaqueCullBack);
  executeRenderingPass(commandBuffer, EDynamicRenderingPass::OpaqueCullNone);
  executeRenderingPass(commandBuffer, EDynamicRenderingPass::DiscardCullNone);
  executeRenderingPass(commandBuffer, EDynamicRenderingPass::BlendCullNone);

  // Synchronize instance processing thread since all instances are submitted queue
  sync.asyncUpdateIndirectDrawBuffers.update();

  // Deferred rendering pass using G-Buffer collected data
  executeRenderingPass(commandBuffer, EDynamicRenderingPass::PBR, material.pGBuffer, true);

  // Additional front rendering passes
  executeRenderingPass(commandBuffer, EDynamicRenderingPass::Skybox);

  executeAOBlurPass(commandBuffer);

  executeRenderingPass(commandBuffer, EDynamicRenderingPass::AlphaCompositing, material.pGPBR, true);

  /* 4. Postprocessing pass */

  // Includes exposure, bloom and TAA passes
  executePostProcessPass(commandBuffer);

  /* 5. Final presentation pass */

  executePresentPass(commandBuffer);

  // End writing commands and prepare to submit buffer to rendering queue
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to end writing to command buffer.");
  }

  // Wait until image to write color data to is acquired
  VkSemaphore waitSems[] = {sync.semImgAvailable[renderView.frameInFlight]};
  VkSemaphore signalSems[] = {sync.semRenderFinished[renderView.frameInFlight]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

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

  sync.asyncUpdateEntities.update();

  renderView.frameInFlight = ++renderView.frameInFlight % MAX_FRAMES_IN_FLIGHT;
  ++renderView.framesRendered;

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
}

void core::MRenderer::renderInitializationFrame() {
  // Generate BRDF lookup table during the initial frame
  queueComputeJob(&compute.environmentJobInfo.LUT);

  // Write ambient occlusion noise/jitter map
  RTexture* pNoiseTexture = core::resources.getTexture(RTGT_NOISEMAP);
  core::resources.writeTexture(pNoiseTexture, system.randomOffsets.data(),
    sizeof(glm::vec4) * system.randomOffsets.size());

  renderFrame();
}