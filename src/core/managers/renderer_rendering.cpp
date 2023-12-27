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

void core::MRenderer::drawBoundEntities(VkCommandBuffer cmdBuffer) {
  // go through bound models and generate draw calls for each
  AEntity* pEntity = nullptr;
  WModel* pModel = nullptr;
  renderView.refresh();

  for (auto& pipeline : renderView.pCurrentRenderPass->usedPipelines) {
    for (auto& bindInfo : system.bindings) {
      if ((pEntity = bindInfo.pEntity) == nullptr) {
        continue;
      }

      if ((pModel = bindInfo.pEntity->getModel()) == nullptr) {
        continue;
      }

      auto& primitives = pModel->getPrimitives();

      for (const auto& primitive : primitives) {
        renderPrimitive(cmdBuffer, primitive, pipeline, &bindInfo);
      }
    }
  }
}

void core::MRenderer::drawBoundEntities(VkCommandBuffer cmdBuffer,
                                        EPipeline forcedPipeline) {
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

  VkPipeline pipeline = getPipeline(pipelineFlag);

  if (renderView.pCurrentPipeline != pipeline) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    renderView.pCurrentPipeline = pipeline;
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
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentRenderPass->usedLayout,
        1, 1, &scene.transformSet, 3, dynamicOffsets);
    renderView.pCurrentMesh = pMesh;
  }

  // bind material descriptor set only if material is different (binding 2)
  if (renderView.pCurrentMaterial != pPrimitive->pMaterial) {
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            renderView.pCurrentRenderPass->usedLayout, 2, 1,
                            &pPrimitive->pMaterial->descriptorSet, 0, nullptr);
    
    // environment render pass uses global push constant block for fragment shader
    if (!renderView.generateEnvironmentMapsImmediate && !renderView.isEnvironmentPass) {
      vkCmdPushConstants(cmdBuffer, renderView.pCurrentRenderPass->usedLayout,
                         VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(RMaterialPCB),
                         &pPrimitive->pMaterial->pushConstantBlock);
    }

    renderView.pCurrentMaterial = pPrimitive->pMaterial;
  }

  int32_t vertexOffset =
      (int32_t)pBindInfo->vertexOffset + (int32_t)pPrimitive->vertexOffset;
  uint32_t indexOffset = pBindInfo->indexOffset + pPrimitive->indexOffset;

  // TODO: implement draw indirect
  vkCmdDrawIndexed(cmdBuffer, pPrimitive->indexCount, 1, indexOffset,
                   vertexOffset, 0);
}

void core::MRenderer::renderEnvironmentMaps(VkCommandBuffer commandBuffer) {
  // initial variables
  uint32_t dynamicOffset = 0, mipLevels = 0, dimension = 0;
  size_t layerOffset = 0;
  size_t layerSize = core::vulkan::envFilterExtent *
                     core::vulkan::envFilterExtent *
                     8;  // 16 bpp RGBA = 8 bytes
  RTexture* pCubemap = nullptr;

  // environment render pass
  RRenderPass* pRenderPass = getRenderPass(ERenderPass::Environment);
  renderView.pCurrentRenderPass = pRenderPass;

  system.renderPassBeginInfo.renderPass =
      renderView.pCurrentRenderPass->renderPass;
  system.renderPassBeginInfo.pClearValues =
      renderView.pCurrentRenderPass->clearValues.data();
  system.renderPassBeginInfo.clearValueCount =
      static_cast<uint32_t>(renderView.pCurrentRenderPass->clearValues.size());

  // render targets must be stored in the same sequence as their related
  // pipelines
  for (uint32_t k = 0; k < pRenderPass->usedPipelines.size(); ++k) {
    switch (pRenderPass->usedPipelines[k]) {
      case EPipeline::EnvFilter: {
        system.renderPassBeginInfo.renderArea.extent = {
            core::vulkan::envFilterExtent, core::vulkan::envFilterExtent};
        pCubemap = environment.destinationTextures[k];  // RTGT_ENVFILTER
        break;
      }
      case EPipeline::EnvIrradiance: {
        system.renderPassBeginInfo.renderArea.extent = {
            core::vulkan::envIrradianceExtent,
            core::vulkan::envIrradianceExtent};
        pCubemap = environment.destinationTextures[k];  // RTGT_ENVIRRAD
        break;
      }
    }

    mipLevels = pCubemap->texture.levelCount;
    dimension = pCubemap->texture.width;

    system.renderPassBeginInfo.framebuffer = system.framebuffers.at(RFB_ENV);

    // start rendering 6 camera views
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pInheritanceInfo = nullptr;
    beginInfo.flags = 0;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      RE_LOG(Error, "failure when trying to record command buffer.");
      return;
    }

    setImageLayout(commandBuffer, pCubemap,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   environment.destinationRanges[k]);

    VkRect2D& scissor = getRenderPass(ERenderPass::Environment)->scissor;
    scissor.extent.width = dimension;
    scissor.extent.height = dimension;

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDeviceSize offset = 0u;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &scene.vertexBuffer.buffer,
                           &offset);
    vkCmdBindIndexBuffer(commandBuffer, scene.indexBuffer.buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    VkViewport& viewport = getRenderPass(ERenderPass::Environment)->viewport;
    viewport.width = static_cast<float>(dimension);
    viewport.height = -static_cast<float>(dimension);
    viewport.y = viewport.width;

    environment.copyRegion.dstSubresource.mipLevel = 0;
    environment.copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
    environment.copyRegion.extent.height =
        static_cast<uint32_t>(-viewport.height);

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // generate default image layers
    for (uint32_t j = 0; j < 6; ++j) {
      dynamicOffset = static_cast<uint32_t>(environment.transformOffset * j);
      layerOffset = layerSize * j;

      vkCmdBeginRenderPass(commandBuffer, &system.renderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_INLINE);
                           
      vkCmdBindDescriptorSets(
          commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
          renderView.pCurrentRenderPass->usedLayout, 0, 1,
          &environment.envDescriptorSets[renderView.frameInFlight], 1,
          &dynamicOffset);

      if (pRenderPass->usedPipelines[k] == EPipeline::EnvFilter) {
        // global set of push constants is used instead of per material ones
        // environment.envPushBlock.roughness = (float)i / (float)(mipLevels -
        // 1);

        vkCmdPushConstants(commandBuffer,
                           renderView.pCurrentRenderPass->usedLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(REnvironmentPCB), &environment.envPushBlock);
      }

      drawBoundEntities(commandBuffer, pRenderPass->usedPipelines[k]);

      vkCmdEndRenderPass(commandBuffer);

      setImageLayout(commandBuffer, environment.pSourceTexture,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     environment.sourceRange);

      // go through cubemap layers and copy front render target
      environment.copyRegion.dstSubresource.baseArrayLayer = j;

      vkCmdCopyImage(commandBuffer, environment.pSourceTexture->texture.image,
                     environment.pSourceTexture->texture.imageLayout,
                     pCubemap->texture.image, pCubemap->texture.imageLayout, 1,
                     &environment.copyRegion);

      setImageLayout(commandBuffer, environment.pSourceTexture,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     environment.sourceRange);
    }

    flushCommandBuffer(commandBuffer, ECmdType::Graphics);

    // generate mipmaps
    generateMipMaps(pCubemap, pCubemap->texture.levelCount);
  }

  // no need to render new environment maps every frame
  renderView.generateEnvironmentMapsImmediate = false;
}

void core::MRenderer::renderEnvironmentMapsSequenced(
    VkCommandBuffer commandBuffer, int32_t frameInterval) {
  if (renderView.framesRendered % frameInterval) return;

  RTexture* pCubemap = nullptr;
  RRenderPass* pRenderPass = getRenderPass(ERenderPass::Environment);

  // if current pipeline exceeds the number of render pass pipelines - reset
  // everything and stop
  if (environment.tracking.pipeline > pRenderPass->usedPipelines.size() - 1) {
    environment.tracking.pipeline = 0;
    environment.tracking.layer = 0;
    environment.tracking.mipLevel = 0;
    renderView.generateEnvironmentMaps = false;
    return;
  }

  // render targets must be stored in the same sequence as their related
  // pipelines
  switch (pRenderPass->usedPipelines[environment.tracking.pipeline]) {
    case EPipeline::EnvFilter: {
      system.renderPassBeginInfo.renderArea.extent = {
          core::vulkan::envFilterExtent, core::vulkan::envFilterExtent};
      pCubemap = environment.destinationTextures[0];  // RTGT_ENVFILTER
      break;
    }
    case EPipeline::EnvIrradiance: {
      system.renderPassBeginInfo.renderArea.extent = {
          core::vulkan::envIrradianceExtent, core::vulkan::envIrradianceExtent};
      pCubemap = environment.destinationTextures[1];  // RTGT_ENVIRRAD
      break;
    }
  }

  // generate the next mip map or render the next face/layer of the cubemap
  if (environment.tracking.mipLevel > 0) {
    if (environment.tracking.mipLevel >= pCubemap->texture.levelCount) {
      environment.tracking.mipLevel = 0;
      ++environment.tracking.layer;

      if (environment.tracking.layer > 5) {
        environment.tracking.layer = 0;
        ++environment.tracking.pipeline;
      }

      return;
    } else {
      VkCommandBufferBeginInfo beginInfo{};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.pInheritanceInfo = nullptr;
      beginInfo.flags = 0;

      if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        RE_LOG(Error, "failure when trying to record command buffer.");
        return;
      }

      generateSingleMipMap(commandBuffer, pCubemap,
                           environment.tracking.mipLevel,
                           environment.tracking.layer);

      flushCommandBuffer(commandBuffer, ECmdType::Graphics);

      ++environment.tracking.mipLevel;

      return;
    }
  }

  uint32_t dynamicOffset = 0, dimension = 0;
  size_t layerOffset = 0;
  size_t layerSize = core::vulkan::envFilterExtent *
                     core::vulkan::envFilterExtent *
                     8;  // 16 bpp RGBA = 8 bytes

  renderView.pCurrentRenderPass = pRenderPass;

  system.renderPassBeginInfo.renderPass =
      renderView.pCurrentRenderPass->renderPass;
  system.renderPassBeginInfo.pClearValues =
      renderView.pCurrentRenderPass->clearValues.data();
  system.renderPassBeginInfo.clearValueCount =
      static_cast<uint32_t>(renderView.pCurrentRenderPass->clearValues.size());

  dimension = pCubemap->texture.width;

  system.renderPassBeginInfo.framebuffer = system.framebuffers.at(RFB_ENV);

  // start rendering an appropriate camera view / layer
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = nullptr;
  beginInfo.flags = 0;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    RE_LOG(Error, "failure when trying to record command buffer.");
    return;
  }

  setImageLayout(commandBuffer, pCubemap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 environment.destinationRanges[environment.tracking.pipeline]);

  VkRect2D& scissor = getRenderPass(ERenderPass::Environment)->scissor;
  scissor.extent.width = dimension;
  scissor.extent.height = dimension;

  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkDeviceSize offset = 0u;
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &scene.vertexBuffer.buffer,
                         &offset);
  vkCmdBindIndexBuffer(commandBuffer, scene.indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);

  VkViewport& viewport = getRenderPass(ERenderPass::Environment)->viewport;
  viewport.width = static_cast<float>(dimension);
  viewport.height = -static_cast<float>(dimension);
  viewport.y = viewport.width;

  environment.copyRegion.dstSubresource.mipLevel = 0;
  environment.copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
  environment.copyRegion.extent.height =
      static_cast<uint32_t>(-viewport.height);

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  // generate default image layers
  dynamicOffset = static_cast<uint32_t>(environment.transformOffset *
                                        environment.tracking.layer);
  layerOffset = layerSize * environment.tracking.layer;

  vkCmdBeginRenderPass(commandBuffer, &system.renderPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      renderView.pCurrentRenderPass->usedLayout, 0, 1,
      &environment.envDescriptorSets[renderView.frameInFlight], 1,
      &dynamicOffset);

  if (pRenderPass->usedPipelines[environment.tracking.pipeline] ==
      EPipeline::EnvFilter) {
    // global set of push constants is used instead of per material ones
    // environment.envPushBlock.roughness = (float)i / (float)(mipLevels -
    // 1);

    vkCmdPushConstants(commandBuffer, renderView.pCurrentRenderPass->usedLayout,
                       VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(REnvironmentPCB),
                       &environment.envPushBlock);
  }
  renderView.isEnvironmentPass = true;
  drawBoundEntities(commandBuffer,
                    pRenderPass->usedPipelines[environment.tracking.pipeline]);
  renderView.isEnvironmentPass = false;
  vkCmdEndRenderPass(commandBuffer);

  setImageLayout(commandBuffer, environment.pSourceTexture,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, environment.sourceRange);

  // go through cubemap layers and copy front render target
  environment.copyRegion.dstSubresource.baseArrayLayer =
      environment.tracking.layer;

  vkCmdCopyImage(commandBuffer, environment.pSourceTexture->texture.image,
                 environment.pSourceTexture->texture.imageLayout,
                 pCubemap->texture.image, pCubemap->texture.imageLayout, 1,
                 &environment.copyRegion);

  setImageLayout(commandBuffer, environment.pSourceTexture,
                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                 environment.sourceRange);

  setImageLayout(commandBuffer, pCubemap,
                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                 environment.destinationRanges[environment.tracking.pipeline]);

  flushCommandBuffer(commandBuffer, ECmdType::Graphics);

  // increase mip level to 1 to generate mip maps next time
  ++environment.tracking.mipLevel;
}

void core::MRenderer::generateLUTMap() {
  core::time.tickTimer();

  RRenderPass* pRenderPass = getRenderPass(ERenderPass::LUTGen);
  renderView.pCurrentRenderPass = pRenderPass;

  system.renderPassBeginInfo.renderPass = getVkRenderPass(ERenderPass::LUTGen);
  system.renderPassBeginInfo.framebuffer = system.framebuffers.at(RFB_LUT);
  system.renderPassBeginInfo.renderArea.extent = {core::vulkan::LUTExtent,
                                                  core::vulkan::LUTExtent};
  system.renderPassBeginInfo.pClearValues =
      renderView.pCurrentRenderPass->clearValues.data();
  system.renderPassBeginInfo.clearValueCount =
      static_cast<uint32_t>(renderView.pCurrentRenderPass->clearValues.size());

  RTexture* pLUTTexture = core::resources.getTexture(RTGT_LUTMAP);

  VkImageSubresourceRange subresourceRange{};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseArrayLayer = 0;
  subresourceRange.layerCount = pLUTTexture->texture.layerCount;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = pLUTTexture->texture.levelCount;

  VkViewport viewport{};
  viewport.width = core::vulkan::LUTExtent;
  viewport.height = -viewport.width;
  viewport.y = viewport.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.extent = system.renderPassBeginInfo.renderArea.extent;
  scissor.offset = {0, 0};

  VkCommandBuffer cmdBuffer = createCommandBuffer(
      ECmdType::Graphics, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  vkCmdBeginRenderPass(cmdBuffer, &system.renderPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
  vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    getPipeline(EPipeline::LUTGen));

  vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(cmdBuffer);

  setImageLayout(cmdBuffer, pLUTTexture,
                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

  flushCommandBuffer(cmdBuffer, ECmdType::Graphics, true, true);

  float timeSpent = core::time.tickTimer();

  // fix this
  RE_LOG(Log, "Generating BRDF LUT map took %.2f milliseconds.", timeSpent);
}

void core::MRenderer::executeRenderPass(VkCommandBuffer commandBuffer,
                                        ERenderPass passType,
                                        VkDescriptorSet* pFrameSets,
                                        const uint32_t setCount,
                                        VkFramebuffer framebuffer) {
  renderView.pCurrentRenderPass = getRenderPass(passType);
  system.renderPassBeginInfo.renderPass = getVkRenderPass(passType);

  system.renderPassBeginInfo.renderPass =
      renderView.pCurrentRenderPass->renderPass;
  system.renderPassBeginInfo.renderArea.extent = {config::renderWidth,
                                                  config::renderHeight};
  system.renderPassBeginInfo.pClearValues =
      renderView.pCurrentRenderPass->clearValues.data();
  system.renderPassBeginInfo.clearValueCount =
      static_cast<uint32_t>(renderView.pCurrentRenderPass->clearValues.size());

  // swapchain image index is different from frame in flight
  system.renderPassBeginInfo.framebuffer = framebuffer;

  vkCmdBeginRenderPass(commandBuffer, &system.renderPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdSetViewport(commandBuffer, 0, 1,
                   &renderView.pCurrentRenderPass->viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchain.imageExtent;

  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkDeviceSize offset = 0u;
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &scene.vertexBuffer.buffer,
                         &offset);
  vkCmdBindIndexBuffer(commandBuffer, scene.indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          renderView.pCurrentRenderPass->usedLayout, 0,
                          setCount, pFrameSets, 0, nullptr);

  drawBoundEntities(commandBuffer);

  vkCmdEndRenderPass(commandBuffer);
}

void core::MRenderer::renderFrame() {
  uint32_t imageIndex = -1;
  TResult chkResult = RE_OK;

  vkWaitForFences(logicalDevice.device, 1,
                  &sync.fenceInFlight[renderView.frameInFlight], VK_TRUE,
                  UINT64_MAX);

  VkResult APIResult =
      vkAcquireNextImageKHR(logicalDevice.device, swapChain, UINT64_MAX,
                            sync.semImgAvailable[renderView.frameInFlight],
                            VK_NULL_HANDLE, &imageIndex);

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

  VkCommandBuffer cmdBuffer = command.buffersGraphics[renderView.frameInFlight];

  if (renderView.generateEnvironmentMaps) {
    renderEnvironmentMapsSequenced(cmdBuffer, environment.genInterval);
  }

  // update view, projection and camera position
  updateSceneUBO(renderView.frameInFlight);

  // update lighting UBO if required
  updateLightingUBO(renderView.frameInFlight);

  VkDescriptorSet frameSets[] = {
      system.descriptorSets[renderView.frameInFlight]};

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = nullptr;
  beginInfo.flags = 0;

  // start recording vulkan command buffer
  if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
    RE_LOG(Error, "failure when trying to record command buffer.");
    return;
  }

  // convert G-buffer render targets to color attachments
  convertRenderTargets(
      cmdBuffer, core::resources.getMaterialTextures(RMAT_GBUFFER), true);

  executeRenderPass(cmdBuffer, ERenderPass::Deferred, frameSets, 1,
                    system.framebuffers.at(RFB_DEFERRED));

  // convert G-buffer render targets to shader read only sources
  convertRenderTargets(
      cmdBuffer, core::resources.getMaterialTextures(RMAT_GBUFFER), false);

  core::world.getModel(RMDL_RENDERPLANE)
      ->setPrimitiveMaterial(0, 0, RMAT_GBUFFER);

  executeRenderPass(cmdBuffer, ERenderPass::PBR, frameSets, 1,
                    system.framebuffers.at(RFB_PBR));

  std::vector<RTexture*> targets;
  targets.emplace_back(core::resources.getTexture(RTGT_GPBR));

  // convert combined PBR output to shader source
  convertRenderTargets(cmdBuffer, &targets, false);

  core::world.getModel(RMDL_RENDERPLANE)
      ->setPrimitiveMaterial(0, 0, RMAT_PRESENT);

  executeRenderPass(cmdBuffer, ERenderPass::Present, frameSets, 1,
                    swapchain.framebuffers[imageIndex]);

  // convert combined PBR output back to render target
  convertRenderTargets(cmdBuffer, &targets, true);

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
  presentInfo.pImageIndices =
      &imageIndex;                 // use selected images for set swapchains
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
  generateLUTMap();
  renderFrame();
}

void core::MRenderer::updateAspectRatio() {
  view.cameraSettings.aspectRatio =
      (float)swapchain.imageExtent.width / swapchain.imageExtent.height;
}

void core::MRenderer::setFOV(float FOV) { view.pActiveCamera->setFOV(FOV); }

void core::MRenderer::setViewDistance(float farZ) { view.cameraSettings.farZ; }

void core::MRenderer::setViewDistance(float nearZ, float farZ) {
  view.cameraSettings.nearZ = nearZ;
  view.cameraSettings.farZ = farZ;
}