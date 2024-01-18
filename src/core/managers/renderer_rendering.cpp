#include "pch.h"
#include "core/core.h"
#include "core/managers/actors.h"
#include "core/managers/animations.h"
#include "core/managers/time.h"
#include "core/managers/world.h"
#include "core/world/actors/entity.h"
#include "core/model/model.h"
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

void core::MRenderer::drawBoundEntities(VkCommandBuffer cmdBuffer,
                                        uint32_t subpassIndex) {
  // go through bound models and generate draw calls for each
  AEntity* pEntity = nullptr;
  WModel* pModel = nullptr;
  renderView.refresh();

  for (auto& pPipeline : renderView.pCurrentRenderPass->pipelines) {
    if (pPipeline->subpassIndex != subpassIndex) {
      continue;
    }

    if (renderView.pCurrentPipeline != pPipeline) {
      renderView.pCurrentPipeline = pPipeline;
      vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->pipeline);
    }

    for (auto& bindInfo : system.bindings) {
      if ((pEntity = bindInfo.pEntity) == nullptr) {
        continue;
      }

      if ((pModel = bindInfo.pEntity->getModel()) == nullptr) {
        continue;
      }

      auto& primitives = pModel->getPrimitives();

      for (const auto& primitive : primitives) {
        renderPrimitive(cmdBuffer, primitive, pPipeline->pipelineId, &bindInfo);
      }
    }
  }
}

void core::MRenderer::drawBoundEntities(VkCommandBuffer cmdBuffer,
                                        EPipeline forcedPipeline) {
  // go through bound models and generate draw calls for each
  AEntity* pEntity = nullptr;
  WModel* pModel = nullptr;
  RPipeline* pPipeline = &getGraphicsPipeline(forcedPipeline);
  renderView.refresh();

  if (renderView.pCurrentPipeline != pPipeline) {
    renderView.pCurrentPipeline = pPipeline;
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->pipeline);
  }

  for (auto& bindInfo : system.bindings) {
    if ((pEntity = bindInfo.pEntity) == nullptr) {
      continue;
    }

    if ((pModel = bindInfo.pEntity->getModel()) == nullptr) {
      continue;
    }

    auto& primitives = pModel->getPrimitives();

    for (const auto& primitive : primitives) {
      renderPrimitive(cmdBuffer, primitive, forcedPipeline, &bindInfo);
    }
  }
}

void core::MRenderer::renderPrimitive(VkCommandBuffer cmdBuffer,
                                      WPrimitive* pPrimitive,
                                      EPipeline pipelineFlag,
                                      REntityBindInfo* pBindInfo) {
  if (!checkPipeline(pPrimitive->pMaterial->pipelineFlags, pipelineFlag)) {
    // does not belong to the current pipeline
    return;
  }

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
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentRenderPass->layout,
        1, 1, &scene.transformDescriptorSet, 3, dynamicOffsets);
    renderView.pCurrentMesh = pMesh;
  }

  // bind material descriptor set only if material is different (binding 2)
  if (renderView.pCurrentMaterial != pPrimitive->pMaterial) {
    // environment render pass uses global push constant block for fragment shader
    if (!renderView.generateEnvironmentMapsImmediate && !renderView.isEnvironmentPass) {
      vkCmdPushConstants(cmdBuffer, renderView.pCurrentRenderPass->layout,
                         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(RSceneVertexPCB), sizeof(RSceneFragmentPCB),
                         &pPrimitive->pMaterial->pushConstantBlock);
    }

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

  RTexture* pCubemap = core::resources.getTexture(RTGT_ENV);
  EViewport viewportIndex = EViewport::vpEnvSkybox;
  auto *pRenderPass = getDynamicRenderingPass(EDynamicRenderingPass::Environment);
  renderView.pCurrentRenderPass = pRenderPass;

  RPipeline* pPipeline = pRenderPass->pipelines.at(0);
  const EPipeline currentPipelineId = pPipeline->pipelineId;

  // start rendering an appropriate camera view / layer
  environment.subresourceRange.baseArrayLayer = environment.tracking.layer;
  setImageLayout(commandBuffer, pCubemap, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, environment.subresourceRange);
  refreshDynamicRenderPass(EDynamicRenderingPass::Environment, 0);

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.colorAttachmentCount = static_cast<uint32_t>(pPipeline->dynamic.colorAttachmentInfo.size());
  renderingInfo.renderArea = system.viewports[viewportIndex].scissor;
  renderingInfo.layerCount = 1;
  renderingInfo.viewMask = 0;

  // modify cubemap color attachment with a required view into an appropriate layer
  VkRenderingAttachmentInfo attachmentInfo = pPipeline->dynamic.colorAttachmentInfo[0];
  attachmentInfo.imageView = pCubemap->texture.cubemapFaceViews[environment.tracking.layer];
  renderingInfo.pColorAttachments = &attachmentInfo;

  vkCmdBeginRendering(commandBuffer, &renderingInfo);

  setViewport(commandBuffer, viewportIndex);

  setCamera(RCAM_ENV);
  getCamera()->setRotation(environment.cameraTransformVectors[environment.tracking.layer]);
  updateSceneUBO(renderView.frameInFlight);

  const uint32_t dynamicOffset = config::scene::cameraBlockSize * view.pActiveCamera->getViewBufferIndex();

  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      renderView.pCurrentRenderPass->layout, 0, 1,
      &scene.descriptorSets[renderView.frameInFlight], 1,
      &dynamicOffset);

  vkCmdPushConstants(commandBuffer, renderView.pCurrentRenderPass->layout,
    VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(RSceneVertexPCB), sizeof(REnvironmentFragmentPCB),
    &environment.pushBlock);

  renderView.isEnvironmentPass = true;
  drawBoundEntities(commandBuffer, pPipeline->pipelineId);
  renderView.isEnvironmentPass = false;
  
  vkCmdEndRendering(commandBuffer);

  setImageLayout(commandBuffer, pCubemap, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, environment.subresourceRange);

  // increase layer count to write to the next cubemap face
  environment.tracking.layer++;
}

void core::MRenderer::executeRenderPass(VkCommandBuffer commandBuffer,
                                        ERenderPass passType,
                                        VkDescriptorSet* pSceneSets,
                                        const uint32_t setCount) {
  ACamera* pCurrentCamera = view.pActiveCamera;

  renderView.pCurrentRenderPass = getRenderPass(passType);
  system.renderPassBeginInfo.renderPass = getVkRenderPass(passType);

  system.renderPassBeginInfo.renderPass =
      renderView.pCurrentRenderPass->renderPass;
  system.renderPassBeginInfo.pClearValues =
      renderView.pCurrentRenderPass->clearValues.data();
  system.renderPassBeginInfo.clearValueCount =
      static_cast<uint32_t>(renderView.pCurrentRenderPass->clearValues.size());

  switch (passType) {
    case ERenderPass::Present: {
      system.renderPassBeginInfo.framebuffer =
          swapchain.framebuffers[renderView.currentFrameIndex].framebuffer;

      vkCmdBeginRenderPass(commandBuffer, &system.renderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_INLINE);

      vkCmdPushConstants(commandBuffer, getPipelineLayout(EPipelineLayout::Scene), VK_SHADER_STAGE_FRAGMENT_BIT,
        sizeof(RSceneVertexPCB), sizeof(RSceneFragmentPCB), &core::resources.getMaterial(RMAT_PRESENT)->pushConstantBlock);

      renderFullscreenQuad(commandBuffer, EPipelineLayout::Scene, EPipeline::Present, nullptr);

      vkCmdEndRenderPass(commandBuffer);

      return;
    }

    case ERenderPass::Shadow: {
      setCamera(view.pSunCamera);
      updateSceneUBO(renderView.frameInFlight);
      system.renderPassBeginInfo.framebuffer =
          renderView.pCurrentRenderPass->pFramebuffer->framebuffer;
      break;
    }

    default: {
      system.renderPassBeginInfo.framebuffer =
          renderView.pCurrentRenderPass->pFramebuffer->framebuffer;
      updateSceneUBO(renderView.frameInFlight);
      break;
    }
  }

  vkCmdBeginRenderPass(commandBuffer, &system.renderPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  const uint32_t dynamicOffset =
      config::scene::cameraBlockSize * view.pActiveCamera->getViewBufferIndex();
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          renderView.pCurrentRenderPass->layout, 0,
                          setCount, pSceneSets, 1, &dynamicOffset);

  drawBoundEntities(commandBuffer);

  switch (passType) {
    case ERenderPass::Shadow: {
      setCamera(pCurrentCamera);
      updateSceneUBO(renderView.frameInFlight);
      break;
    }
    case ERenderPass::Deferred: {
      // Subpass 1
      vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

      renderFullscreenQuad(commandBuffer, EPipelineLayout::PBR,
                           EPipeline::PBR, &scene.GBufferDescriptorSet,
                           pSceneSets, dynamicOffset);

      // Subpass 2
      vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              getPipelineLayout(EPipelineLayout::Scene), 0,
                              setCount, pSceneSets, 1, &dynamicOffset);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderView.pCurrentRenderPass->layout, 2, 1,
        &material.descriptorSet, 0, nullptr);

      drawBoundEntities(commandBuffer, 2u);

      break;
    }
  }

  vkCmdEndRenderPass(commandBuffer);
}

void core::MRenderer::executeDynamicRendering(VkCommandBuffer commandBuffer,
                                               EDynamicRenderingPass renderPass) {
  //vkCmdBeginRendering(commandBuffer, pRenderingInfo);
  //drawBoundEntities(commandBuffer, pipeline);
  //vkCmdEndRendering(commandBuffer);
}

void core::MRenderer::renderFullscreenQuad(VkCommandBuffer commandBuffer,
                                           EPipelineLayout pipelineLayout,
                                           EPipeline pipeline,
                                           VkDescriptorSet* pAttachmentSet,
                                           VkDescriptorSet* pSceneSet,
                                           uint32_t sceneDynamicOffset) {
  RPipeline* pPipeline = &getGraphicsPipeline(pipeline);

  if (renderView.pCurrentPipeline != pPipeline) {
    renderView.pCurrentPipeline = pPipeline;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      getGraphicsPipeline(pipeline).pipeline);
  }

  if (pSceneSet) {
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            getPipelineLayout(pipelineLayout), 0, 1, pSceneSet,
                            1, &sceneDynamicOffset);
  }

  if (pAttachmentSet) {
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      getPipelineLayout(pipelineLayout), 2, 1,
      pAttachmentSet, 0, nullptr);
  }

  vkCmdDraw(commandBuffer, 6, 1, 0, 0);
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

  VkDescriptorSet frameSets[] = {
      scene.descriptorSets[renderView.frameInFlight]};

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
    setCamera(RCAM_MAIN);
  }

  setViewport(cmdBuffer, EViewport::vpShadow);
  executeRenderPass(cmdBuffer, ERenderPass::Shadow, frameSets, 1);

  setViewport(cmdBuffer, EViewport::vpMain);
  executeRenderPass(cmdBuffer, ERenderPass::Deferred, frameSets, 1);
  executeRenderPass(cmdBuffer, ERenderPass::Present, frameSets, 1);

  // end writing commands and prepare to submit buffer to rendering queue
  if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to end writing to command buffer.");
  }

  // wait until image to write color data to is acquired
  VkSemaphore waitSems[] = {sync.semImgAvailable[renderView.frameInFlight]};
  VkSemaphore signalSems[] = {sync.semRenderFinished[renderView.frameInFlight]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSems;
  submitInfo.pWaitDstStageMask =
      waitStages;  // each stage index corresponds to provided semaphore index
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers =
      &command
           .buffersGraphics[renderView.frameInFlight];  // submit command buffer
                                                        // recorded previously
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores =
      signalSems;  // signal these after rendering is finished

  // submit an array featuring command buffers to graphics queue and signal
  // fence for CPU to wait for execution
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
  presentInfo.pWaitSemaphores =
      signalSems;  // present when render finished semaphores are signaled
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &renderView.currentFrameIndex;   // use selected images for set swapchains
  presentInfo.pResults = nullptr;  // for future use with more swapchains

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