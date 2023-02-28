#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/time.h"
#include "core/world/model/primitive.h"

TResult core::MRenderer::createRenderPass() {
  RE_LOG(Log, "Creating render pass");

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapchain.formatData.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                 // clearing contents on new frame
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;               // storing contents in memory while rendering
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;      // ignore stencil buffer
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;            // not important, since it's cleared at the frame start
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;        // swap chain image to be presented

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;                                    // index of an attachment in attachmentdesc array (also vertex shader output index)
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // the layout will be an image

  VkSubpassDescription subpassDesc{};
  subpassDesc.colorAttachmentCount = 1;
  subpassDesc.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};                                  // makes subpass wait for color attachment-type image to be acquired
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;   // wait for external subpass to be of color attachment
  dependency.srcAccessMask = NULL;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;   // wait until we can write to color attachment
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDesc;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(logicalDevice.device, &renderPassInfo, nullptr,
                         &system.renderPass) != VK_SUCCESS) {
    RE_LOG(Critical, "render pass creation failed.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MRenderer::destroyRenderPass() {
  RE_LOG(Log, "Destroying render pass.");
  vkDestroyRenderPass(logicalDevice.device, system.renderPass, nullptr);
}

TResult core::MRenderer::createGraphicsPipeline() {
  RE_LOG(Log, "Creating graphics pipeline.");

  std::vector<char> vsCode = readFile(TEXT("content/shaders/vs_default.spv"));
  std::vector<char> fsCode = readFile(TEXT("content/shaders/fs_default.spv"));
  VkShaderModule vsModule = createShaderModule(vsCode);
  VkShaderModule fsModule = createShaderModule(fsCode);

  VkPipelineShaderStageCreateInfo vsStageInfo{};
  vsStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vsStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vsStageInfo.module = vsModule;
  vsStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fsStageInfo{};
  fsStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fsStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fsStageInfo.module = fsModule;
  fsStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vsStageInfo, fsStageInfo};

  auto bindingDesc = RVertex::getBindingDesc();
  auto attributeDescs = RVertex::getAttributeDescs();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescs.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  inputAssemblyInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapchain.imageExtent.width;
  viewport.height = (float)swapchain.imageExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchain.imageExtent;

  VkPipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportInfo.pViewports = &viewport;
  viewportInfo.viewportCount = 1;
  viewportInfo.pScissors = &scissor;
  viewportInfo.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
  rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationInfo.depthClampEnable = VK_FALSE;
  rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationInfo.lineWidth = 1.0f;
  rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizationInfo.depthBiasEnable = VK_FALSE;
  rasterizationInfo.depthBiasConstantFactor = 0.0f;
  rasterizationInfo.depthBiasClamp = 0.0f;
  rasterizationInfo.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampleInfo{};
  multisampleInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleInfo.sampleShadingEnable = VK_FALSE;
  multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampleInfo.minSampleShading = 1.0f;
  multisampleInfo.pSampleMask = nullptr;
  multisampleInfo.alphaToCoverageEnable = VK_FALSE;
  multisampleInfo.alphaToOneEnable = VK_FALSE;

  VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
  depthStencilInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilInfo.depthTestEnable = VK_TRUE;
  depthStencilInfo.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
  depthStencilInfo.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
  colorBlendInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendInfo.logicOpEnable = VK_FALSE;
  colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
  colorBlendInfo.attachmentCount = 1;
  colorBlendInfo.pAttachments = &colorBlendAttachment;
  colorBlendInfo.blendConstants[0] = 0.0f;
  colorBlendInfo.blendConstants[1] = 0.0f;
  colorBlendInfo.blendConstants[2] = 0.0f;
  colorBlendInfo.blendConstants[3] = 0.0f;

  // dynamic states allow changing specific parts of the pipeline without recreating it
  std::vector<VkDynamicState> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynStateInfo{};
  dynStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynStateInfo.pDynamicStates = dynamicStates.data();

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &system.descriptorSetLayout;
  layoutInfo.pushConstantRangeCount = 0;
  layoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(logicalDevice.device, &layoutInfo, nullptr,
                             &system.pipelineLayout) != VK_SUCCESS) {
    RE_LOG(Critical, "failed to create graphics pipeline layout.");

    return RE_CRITICAL;
  }

  VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
  graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  graphicsPipelineInfo.pColorBlendState = &colorBlendInfo;
  //graphicsPipelineInfo.pDepthStencilState = &depthStencilInfo;
  graphicsPipelineInfo.pDepthStencilState = nullptr;
  graphicsPipelineInfo.pDynamicState = &dynStateInfo;
  graphicsPipelineInfo.pMultisampleState = &multisampleInfo;
  graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
  graphicsPipelineInfo.pRasterizationState = &rasterizationInfo;
  graphicsPipelineInfo.pViewportState = &viewportInfo;
  graphicsPipelineInfo.stageCount = 2;
  graphicsPipelineInfo.pStages = shaderStages;
  graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
  graphicsPipelineInfo.layout = system.pipelineLayout;
  graphicsPipelineInfo.renderPass = system.renderPass;
  graphicsPipelineInfo.subpass = 0;                           // subpass index for render pass
  graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  graphicsPipelineInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(logicalDevice.device, VK_NULL_HANDLE, 1,
                                &graphicsPipelineInfo, nullptr,
                                &system.pipeline) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create graphics pipeline.");

    return RE_CRITICAL;
  }

  vkDestroyShaderModule(logicalDevice.device, vsModule, nullptr);
  vkDestroyShaderModule(logicalDevice.device, fsModule, nullptr);

  return RE_OK;
}

void core::MRenderer::destroyGraphicsPipeline() {
  RE_LOG(Log, "Shutting down graphics pipeline.");
  destroyDescriptorSetLayouts();
  vkDestroyPipeline(logicalDevice.device, system.pipeline, nullptr);
  vkDestroyPipelineLayout(logicalDevice.device, system.pipelineLayout, nullptr);
}

TResult core::MRenderer::createCommandPools() {
  RE_LOG(Log, "Creating command pool.");

  VkCommandPoolCreateInfo cmdPoolRenderInfo{};
  cmdPoolRenderInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolRenderInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cmdPoolRenderInfo.queueFamilyIndex =
      physicalDevice.queueFamilyIndices.graphics[0];

  if (vkCreateCommandPool(logicalDevice.device, &cmdPoolRenderInfo, nullptr,
                          &command.poolRender) != VK_SUCCESS) {
    RE_LOG(Critical,
           "failed to create command pool for graphics queue family.");

    return RE_CRITICAL;
  }

  cmdPoolRenderInfo.queueFamilyIndex =
      physicalDevice.queueFamilyIndices.transfer[0];

  if (vkCreateCommandPool(logicalDevice.device, &cmdPoolRenderInfo, nullptr,
                          &command.poolTransfer) != VK_SUCCESS) {
    RE_LOG(Critical,
           "failed to create command pool for graphics queue family.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MRenderer::destroyCommandPools() {
  RE_LOG(Log, "Destroying command pools.");
  vkDestroyCommandPool(logicalDevice.device, command.poolRender, nullptr);
  vkDestroyCommandPool(logicalDevice.device, command.poolTransfer, nullptr);
}

TResult core::MRenderer::createCommandBuffers() {
  RE_LOG(Log, "Creating rendering command buffers for %d frames.",
         MAX_FRAMES_IN_FLIGHT);

  command.bufferView.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo cmdBufferInfo{};
  cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdBufferInfo.commandPool = command.poolRender;
  cmdBufferInfo.commandBufferCount =
      (uint32_t)command.bufferView.size();

  if (vkAllocateCommandBuffers(logicalDevice.device, &cmdBufferInfo,
                               command.bufferView.data()) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to allocate rendering command buffers.");
    return RE_CRITICAL;
  }

  RE_LOG(Log, "Creating %d transfer command buffers.", MAX_TRANSFER_BUFFERS);

  command.buffersTransfer.resize(MAX_TRANSFER_BUFFERS);

  cmdBufferInfo.commandPool = command.poolTransfer;
  cmdBufferInfo.commandBufferCount =
      (uint32_t)command.buffersTransfer.size();

  if (vkAllocateCommandBuffers(logicalDevice.device, &cmdBufferInfo,
                               command.buffersTransfer.data()) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to allocate transfer command buffers.");
    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MRenderer::destroyCommandBuffers() {
  RE_LOG(Log, "Freeing %d rendering command buffers.",
         command.bufferView.size());
  vkFreeCommandBuffers(logicalDevice.device, command.poolRender,
                       static_cast<uint32_t>(command.bufferView.size()),
                       command.bufferView.data());

  RE_LOG(Log, "Freeing %d transfer command buffer.",
         command.buffersTransfer.size());
  vkFreeCommandBuffers(logicalDevice.device, command.poolTransfer,
                       static_cast<uint32_t>(command.buffersTransfer.size()),
                       command.buffersTransfer.data());
}

TResult core::MRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                     uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = nullptr;
  beginInfo.flags = 0;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    RE_LOG(Error, "failure when trying to record command buffer.");

    return RE_ERROR;
  }

  VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = system.renderPass;
  renderPassInfo.framebuffer = swapchain.framebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapchain.imageExtent;
  renderPassInfo.pClearValues = &clearColor;
  renderPassInfo.clearValueCount = 1;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    system.pipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = (core::vulkan::bFlipViewPortY)
                   ? static_cast<float>(swapchain.imageExtent.height)
                   : 0.0f;
  viewport.width = static_cast<float>(swapchain.imageExtent.width);
  viewport.height = (core::vulkan::bFlipViewPortY)
                        ? -static_cast<float>(swapchain.imageExtent.height)
                        : static_cast<float>(swapchain.imageExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchain.imageExtent;
  
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkBuffer vertexBuffers[] = {system.meshes[0]->vertexBuffer.buffer};
  VkDeviceSize offsets[] = {0};
  uint32_t numVertices = system.meshes[0]->vertexCount;
  uint32_t numIndices = system.meshes[0]->indexCount;

  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  vkCmdBindIndexBuffer(commandBuffer, system.meshes[0]->indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          system.pipelineLayout, 0, 1,
                          &system.descriptorSets[system.idIFFrame], 0, nullptr);

  vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to end writing to command buffer.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

TResult core::MRenderer::createSyncObjects() {
  RE_LOG(Log, "Creating sychronization objects for %d frames.",
         MAX_FRAMES_IN_FLIGHT);

  sync.semImgAvailable.resize(MAX_FRAMES_IN_FLIGHT);
  sync.semRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);
  sync.fenceInFlight.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semInfo{};
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenInfo{};
  fenInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // signaled to skip waiting for
                                                 // it on the first frame

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (vkCreateSemaphore(logicalDevice.device, &semInfo, nullptr,
                          &sync.semImgAvailable[i]) != VK_SUCCESS) {
      RE_LOG(Critical, "failed to create 'image available' semaphore.");

      return RE_CRITICAL;
    }

    if (vkCreateSemaphore(logicalDevice.device, &semInfo, nullptr,
                          &sync.semRenderFinished[i]) != VK_SUCCESS) {
      RE_LOG(Critical, "failed to create 'render finished' semaphore.");

      return RE_CRITICAL;
    }

    if (vkCreateFence(logicalDevice.device, &fenInfo, nullptr,
                      &sync.fenceInFlight[i])) {
      RE_LOG(Critical, "failed to create 'in flight' fence.");

      return RE_CRITICAL;
    }
  }

  return RE_OK;
}

void core::MRenderer::destroySyncObjects() {
  RE_LOG(Log, "Destroying synchronization objects.");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(logicalDevice.device, sync.semImgAvailable[i],
                       nullptr);
    vkDestroySemaphore(logicalDevice.device, sync.semRenderFinished[i],
                       nullptr);

    vkDestroyFence(logicalDevice.device, sync.fenceInFlight[i],
                   nullptr);
  }
}

TResult core::MRenderer::drawFrame() {
  uint32_t imageIndex = -1;
  TResult chkResult = RE_OK;

  vkWaitForFences(logicalDevice.device, 1,
                  &sync.fenceInFlight[system.idIFFrame], VK_TRUE,
                  UINT64_MAX);

  VkResult APIResult =
      vkAcquireNextImageKHR(logicalDevice.device, swapChain, UINT64_MAX,
                            sync.semImgAvailable[system.idIFFrame],
                            VK_NULL_HANDLE, &imageIndex);

  if (APIResult == VK_ERROR_OUT_OF_DATE_KHR) {
    RE_LOG(Warning, "Out of date swap chain image. Recreating swap chain.");

    recreateSwapChain();
    return RE_WARNING;
  }

  if (APIResult != VK_SUCCESS && APIResult != VK_SUBOPTIMAL_KHR) {
    RE_LOG(Error, "Failed to acquire valid swap chain image.");

    return RE_ERROR;
  }

  // get new delta time between frames
  core::time.tickTimer();

  // update MVP buffers
  updateModelViewProjectionBuffers(system.idIFFrame);

  // reset fences if we will do any work this frame e.g. no swap chain
  // recreation
  vkResetFences(logicalDevice.device, 1,
                &sync.fenceInFlight[system.idIFFrame]);

  vkResetCommandBuffer(command.bufferView[system.idIFFrame], NULL);

  // generate frame data
  recordCommandBuffer(command.bufferView[system.idIFFrame], imageIndex);

  // wait until image to write color data to is acquired
  VkSemaphore waitSems[] = {sync.semImgAvailable[system.idIFFrame]};
  VkSemaphore signalSems[] = {
      sync.semRenderFinished[system.idIFFrame]};
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
      &command.bufferView[system.idIFFrame];  // submit command buffer
                                                   // recorded previously
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores =
      signalSems;  // signal these after rendering is finished

  // submit an array featuring command buffers to graphics queue and signal
  // fence for CPU to wait for execution
  if (vkQueueSubmit(logicalDevice.queues.graphics, 1, &submitInfo,
                    sync.fenceInFlight[system.idIFFrame]) !=
      VK_SUCCESS) {
    RE_LOG(Error, "Failed to submit data to graphics queue.");

    chkResult = RE_ERROR;
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
      bFramebufferResized) {

    RE_LOG(Warning, "Recreating swap chain, reason: %d", APIResult);
    bFramebufferResized = false;
    recreateSwapChain();

    return RE_WARNING;
  }

  if (APIResult != VK_SUCCESS) {
    RE_LOG(Error, "Failed to present new frame.");

    return RE_ERROR;
  }

  system.idIFFrame = ++system.idIFFrame % MAX_FRAMES_IN_FLIGHT;

  return chkResult;
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