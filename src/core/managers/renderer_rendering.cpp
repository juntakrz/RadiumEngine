#include "pch.h"
#include "core/core.h"
#include "core/managers/actors.h"
#include "core/managers/animations.h"
#include "core/managers/time.h"
#include "core/managers/world.h"
#include "core/world/actors/entity.h"
#include "core/model/model.h"
#include "core/material/texture.h"
#include "core/managers/renderer.h"

// runs in a dedicated thread
void core::MRenderer::updateBoundEntities() {
  AEntity* pEntity = nullptr;

  // update animation matrices
  core::animations.runAnimationQueue();

  for (auto& bindInfo : system.bindings) {
    if ((pEntity = bindInfo.pEntity) == nullptr) {
      continue;
    }

    // update model matrices
    pEntity->updateModel();
  }
}

void core::MRenderer::drawBoundEntities(VkCommandBuffer commandBuffer) {
  // go through bound models and generate draw calls for each
  AEntity* pEntity = nullptr;
  WModel* pModel = nullptr;
  renderView.refresh();

  for (auto& bindInfo : system.bindings) {
    if ((pEntity = bindInfo.pEntity) == nullptr) {
      continue;
    }

    if ((pModel = bindInfo.pEntity->getModel()) == nullptr) {
      continue;
    }

    auto& primitives = pModel->getPrimitives();

    for (const auto& primitive : primitives) {
      if (!checkPass(primitive->pMaterial->passFlags, renderView.pCurrentPass->passId)) continue;

      renderPrimitive(commandBuffer, primitive, &bindInfo);
    }
  }
}

void core::MRenderer::renderPrimitive(VkCommandBuffer cmdBuffer,
                                      WPrimitive* pPrimitive,
                                      REntityBindInfo* pBindInfo) {

  WModel::Node* pNode = reinterpret_cast<WModel::Node*>(pPrimitive->pOwnerNode);
  WModel::Mesh* pMesh = pNode->pMesh.get();
  AEntity* pEntity = pBindInfo->pEntity;

  // mesh descriptor set is at binding 1 (TODO: change from mesh to actor)
  if (renderView.pCurrentMesh != pMesh) {
    uint32_t skinOffset =
        (pNode->pSkin) ? static_cast<uint32_t>(pNode->pSkin->bufferOffset) : 0u;
    uint32_t dynamicOffsets[3] = {
        pEntity->getRootTransformBufferOffset(),
        pEntity->getNodeTransformBufferOffset(pNode->index), skinOffset};
    vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->layout,
        1, 1, &scene.transformDescriptorSet, 3, dynamicOffsets);
    renderView.pCurrentMesh = pMesh;
  }

  // bind material descriptor set only if material is different (binding 2)
  if (renderView.pCurrentMaterial != pPrimitive->pMaterial) {
    vkCmdPushConstants(cmdBuffer, renderView.pCurrentPass->layout,
                        VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(RSceneVertexPCB), sizeof(RSceneFragmentPCB),
                        &pPrimitive->pMaterial->pushConstantBlock);

    renderView.pCurrentMaterial = pPrimitive->pMaterial;
  }

  int32_t vertexOffset =
      (int32_t)pBindInfo->vertexOffset + (int32_t)pPrimitive->vertexOffset;
  uint32_t indexOffset = pBindInfo->indexOffset + pPrimitive->indexOffset;

  // TODO: implement draw indirect
  vkCmdDrawIndexed(cmdBuffer, pPrimitive->indexCount, 1, indexOffset, vertexOffset, 0);
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

    createComputeJob(&environment.computeJobs.prefiltered);
    environment.tracking.layer++;
    return;

  // All 6 faces are rendered, compute irradiance map one layer per frame
  } else if (environment.tracking.layer > 5) {
    // Cubemap face index
    environment.computeJobs.irradiance.intValues.y = environment.tracking.layer - 6;

    createComputeJob(&environment.computeJobs.irradiance);
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

void core::MRenderer::executeDynamicRenderingPass(VkCommandBuffer commandBuffer, EDynamicRenderingPass passId, VkDescriptorSet sceneSet,
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
  // TODO:
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
                          1, &sceneSet, 1, &dynamicOffset);

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

void core::MRenderer::executeDynamicShadowPass(VkCommandBuffer commandBuffer, const uint32_t cascadeIndex, VkDescriptorSet sceneSet) {
  RDynamicRenderingPass* pRenderPass = getDynamicRenderingPass(EDynamicRenderingPass::Shadow);
  renderView.pCurrentPass = pRenderPass;

  VkRenderingAttachmentInfo overrideAttachment = *pRenderPass->renderingInfo.pDepthAttachment;
  overrideAttachment.imageView = pRenderPass->pImageReferences[0]->texture.extraViews[cascadeIndex].imageView;

  VkRenderingInfo overrideInfo{};
  overrideInfo = pRenderPass->renderingInfo;
  overrideInfo.pDepthAttachment = &overrideAttachment;

  setCamera(view.pSunCamera);
  updateSceneUBO(renderView.frameInFlight);

  scene.vertexPushBlock.cascadeIndex = cascadeIndex;

  vkCmdBeginRendering(commandBuffer, &overrideInfo);

  setViewport(commandBuffer, renderView.pCurrentPass->viewportId);
  renderView.currentViewportId = renderView.pCurrentPass->viewportId;

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->pipeline);

  const uint32_t dynamicOffset = config::scene::cameraBlockSize * view.pActiveCamera->getViewBufferIndex();

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentPass->layout, 0,
    1, &sceneSet, 1, &dynamicOffset);

  vkCmdPushConstants(commandBuffer, renderView.pCurrentPass->layout, VK_SHADER_STAGE_VERTEX_BIT, 0u,
                     sizeof(RSceneVertexPCB), &scene.vertexPushBlock);

  drawBoundEntities(commandBuffer);

  vkCmdEndRendering(commandBuffer);
}

void core::MRenderer::executeDynamicPresentPass(VkCommandBuffer commandBuffer, VkDescriptorSet sceneSet) {
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

  executeComputeJobs();

  VkCommandBuffer cmdBuffer = command.buffersGraphics[renderView.frameInFlight];

  // update lighting UBO if required
  updateLightingUBO(renderView.frameInFlight);

  VkDescriptorSet frameSet = scene.descriptorSets[renderView.frameInFlight];

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = nullptr;
  beginInfo.flags = 0;

  // start recording vulkan command buffer
  if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
    RE_LOG(Error, "Failure when trying to record command buffer.");
    return;
  }

  VkDeviceSize vbOffset = 0u;
  vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &scene.vertexBuffer.buffer, &vbOffset);
  vkCmdBindIndexBuffer(cmdBuffer, scene.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    getPipelineLayout(EPipelineLayout::Scene), 2, 1, &material.descriptorSet, 0, nullptr);

  if (renderView.generateEnvironmentMaps) {
    renderEnvironmentMaps(cmdBuffer, environment.genInterval);
  }

  for (uint8_t cascadeIndex = 0; cascadeIndex < config::shadowCascades; ++cascadeIndex) {
    executeDynamicShadowPass(cmdBuffer, cascadeIndex, frameSet);
  }

  setCamera(RCAM_MAIN);
  updateSceneUBO(renderView.frameInFlight);

  // G-Buffer passes
  executeDynamicRenderingPass(cmdBuffer, EDynamicRenderingPass::OpaqueCullBack, frameSet);
  executeDynamicRenderingPass(cmdBuffer, EDynamicRenderingPass::OpaqueCullNone, frameSet);
  executeDynamicRenderingPass(cmdBuffer, EDynamicRenderingPass::BlendCullNone, frameSet);

  // Deferred rendering pass using G-Buffer collected data
  executeDynamicRenderingPass(cmdBuffer, EDynamicRenderingPass::PBR, frameSet, material.pGBuffer, true);

  // Additional front rendering passes
  executeDynamicRenderingPass(cmdBuffer, EDynamicRenderingPass::Skybox, frameSet);

  // Final presentation pass
  executeDynamicPresentPass(cmdBuffer, frameSet);

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
    ;
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

  sync.asyncUpdateEntities.update();

  renderView.frameInFlight = ++renderView.frameInFlight % MAX_FRAMES_IN_FLIGHT;
  ++renderView.framesRendered;
}

void core::MRenderer::renderInitFrame() {
  // Generate BRDF LUT during the initial frame
  createComputeJob(&environment.computeJobs.LUT);

  renderFrame();
}