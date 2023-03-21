#include "pch.h"
#include "core/core.h"
#include "core/managers/time.h"
#include "core/world/actors/entity.h"
#include "core/model/model.h"
#include "core/managers/renderer.h"

// runs in a second thread
void core::MRenderer::updateBoundEntities() {
  AEntity* pEntity = nullptr;

  for (auto& bindInfo : system.bindings) {
    if ((pEntity = bindInfo.pEntity) == nullptr) {
      continue;
    }

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

void core::MRenderer::renderPrimitive(VkCommandBuffer cmdBuffer,
                                      WPrimitive* pPrimitive,
                                      EPipeline pipelineFlag,
                                      REntityBindInfo* pBindInfo) {
  if (!checkPipeline(pPrimitive->pMaterial->pipelineFlags, pipelineFlag)) {
    // does not belong to the current pipeline
    return;
  }

  VkPipeline pipeline = getPipeline(pipelineFlag);

  if (pipeline == VK_NULL_HANDLE) {
    // pipeline wasn't resolved because it probably isn't implemented yet
    return;
  }

  if (renderView.pCurrentPipeline != pipeline) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    renderView.pCurrentPipeline = pipeline;
  }

  WModel::Node* pNode = reinterpret_cast<WModel::Node*>(pPrimitive->pOwnerNode);
  WModel::Mesh* pMesh = pNode->pMesh.get();

  // mesh descriptor set is at binding 1
  if (renderView.pCurrentMesh != pMesh) {
    vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderView.pCurrentRenderPass->usedLayout,
        1, 1, &pMesh->uniformBufferData.descriptorSet, 0, nullptr);
    renderView.pCurrentMesh = pMesh;
  }

  // bind material descriptor set only if material is different (binding 2)
  if (renderView.pCurrentMaterial != pPrimitive->pMaterial) {
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            renderView.pCurrentRenderPass->usedLayout, 2, 1,
                            &pPrimitive->pMaterial->descriptorSet, 0, nullptr);
                            
    vkCmdPushConstants(cmdBuffer, renderView.pCurrentRenderPass->usedLayout,
                       VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(RMaterialPCB),
                       &pPrimitive->pMaterial->pushConstantBlock);

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
  uint32_t dynamicOffset =
      static_cast<uint32_t>(environment.transformOffset * 0);

  // environment render pass
  renderView.pCurrentRenderPass = getRenderPass(ERenderPass::Environment);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = nullptr;
  beginInfo.flags = 0;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    RE_LOG(Error, "failure when trying to record command buffer.");
    return;
  }

  system.renderPassBeginInfo.renderPass =
      renderView.pCurrentRenderPass->renderPass;
  system.renderPassBeginInfo.renderArea.extent = {
      core::vulkan::envCubeResolution, core::vulkan::envCubeResolution
  };

  system.renderPassBeginInfo.framebuffer = system.framebuffers.at(FB_FRONT);

  vkCmdBeginRenderPass(commandBuffer, &system.renderPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdSetViewport(commandBuffer, 0, 1,
                   &renderView.pCurrentRenderPass->viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent.width = core::resources.getTexture(RT_CUBEMAP)->texture.width;
  scissor.extent.height = core::resources.getTexture(RT_CUBEMAP)->texture.height;

  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkDeviceSize offset = 0u;
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &scene.vertexBuffer.buffer,
                         &offset);
  vkCmdBindIndexBuffer(commandBuffer, scene.indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          renderView.pCurrentRenderPass->usedLayout, 0, 1,
                          &environment.descriptorSets[renderView.frameInFlight],
                          1, &dynamicOffset);

  drawBoundEntities(commandBuffer);

  vkCmdEndRenderPass(commandBuffer);

  /*if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to end writing to command buffer.");
  }*/

  flushCommandBuffer(commandBuffer, ECmdType::Graphics, false, true);

  // no need to render new environment maps every frame
  renderView.doEnvironmentPass = false;
}

void core::MRenderer::doRenderPass(VkCommandBuffer commandBuffer,
                                   std::vector<VkDescriptorSet>& sets,
                                   uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = nullptr;
  beginInfo.flags = 0;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    RE_LOG(Error, "failure when trying to record command buffer.");
    return;
  }

  system.renderPassBeginInfo.renderPass =
      renderView.pCurrentRenderPass->renderPass;
  system.renderPassBeginInfo.renderArea.extent = {config::renderWidth,
                                                  config::renderHeight};

  // swapchain image index is different from frame in flight
  system.renderPassBeginInfo.framebuffer = swapchain.framebuffers[imageIndex];

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
                          renderView.pCurrentRenderPass->usedLayout, 0, 1,
                          &sets[renderView.frameInFlight], 0, nullptr);

  drawBoundEntities(commandBuffer);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to end writing to command buffer.");
  }
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

  if (renderView.doEnvironmentPass) {
    //renderEnvironmentMaps(cmdBuffer);
  }
  renderEnvironmentMaps(cmdBuffer);
  // main PBR render pass:
  // update view, projection and camera position
  updateSceneUBO(renderView.frameInFlight);
  renderView.pCurrentRenderPass = getRenderPass(ERenderPass::PBR);
  //doRenderPass(cmdBuffer, system.descriptorSets, imageIndex);

  // wait until image to write color data to is acquired
  VkSemaphore waitSems[] = {sync.semImgAvailable[renderView.frameInFlight]};
  VkSemaphore signalSems[] = {
      sync.semRenderFinished[renderView.frameInFlight]};
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
      &command.buffersGraphics[renderView.frameInFlight];  // submit command buffer
                                                   // recorded previously
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores =
      signalSems;  // signal these after rendering is finished

  // submit an array featuring command buffers to graphics queue and signal
  // fence for CPU to wait for execution
  if (vkQueueSubmit(logicalDevice.queues.graphics, 1, &submitInfo,
                    sync.fenceInFlight[renderView.frameInFlight]) !=
      VK_SUCCESS) {
    RE_LOG(Error, "Failed to submit data to graphics queue.");;
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

void core::MRenderer::updateAspectRatio() {
  view.cameraSettings.aspectRatio =
      (float)swapchain.imageExtent.width / swapchain.imageExtent.height;
}

void core::MRenderer::setFOV(float FOV) { view.cameraSettings.FOV = FOV; }

void core::MRenderer::setViewDistance(float farZ) { view.cameraSettings.farZ; }

void core::MRenderer::setViewDistance(float nearZ, float farZ) {
  view.cameraSettings.nearZ = nearZ;
  view.cameraSettings.farZ = farZ;
}