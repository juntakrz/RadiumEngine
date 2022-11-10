#include "pch.h"
#include "core/managers/mgraphics.h"
#include "core/world/mesh/mesh.h"

TResult MGraphics::createRenderPass() {
  RE_LOG(Log, "Creating render pass");

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = dataSwapChain.formatData.format;
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
                         &dataSystem.renderPass) != VK_SUCCESS) {
    RE_LOG(Critical, "render pass creation failed.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

void MGraphics::destroyRenderPass() {
  RE_LOG(Log, "Destroying render pass.");
  vkDestroyRenderPass(logicalDevice.device, dataSystem.renderPass, nullptr);
}

TResult MGraphics::createGraphicsPipeline() {
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
  viewport.width = (float)dataSwapChain.imageExtent.width;
  viewport.height = (float)dataSwapChain.imageExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = dataSwapChain.imageExtent;

  VkPipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportInfo.pViewports = &viewport;
  viewportInfo.viewportCount = 1;
  viewportInfo.pScissors = &scissor;
  viewportInfo.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterInfo{};
  rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterInfo.depthClampEnable = VK_FALSE;
  rasterInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterInfo.lineWidth = 1.0f;
  rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterInfo.depthBiasEnable = VK_FALSE;
  rasterInfo.depthBiasConstantFactor = 0.0f;
  rasterInfo.depthBiasClamp = 0.0f;
  rasterInfo.depthBiasSlopeFactor = 0.0f;

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
  layoutInfo.setLayoutCount = 0;
  layoutInfo.pSetLayouts = nullptr;
  layoutInfo.pushConstantRangeCount = 0;
  layoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(logicalDevice.device, &layoutInfo, nullptr,
                             &dataSystem.pipelineLayout) != VK_SUCCESS) {
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
  graphicsPipelineInfo.pRasterizationState = &rasterInfo;
  graphicsPipelineInfo.pViewportState = &viewportInfo;
  graphicsPipelineInfo.stageCount = 2;
  graphicsPipelineInfo.pStages = shaderStages;
  graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
  graphicsPipelineInfo.layout = dataSystem.pipelineLayout;
  graphicsPipelineInfo.renderPass = dataSystem.renderPass;
  graphicsPipelineInfo.subpass = 0;                           // subpass index for render pass
  graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  graphicsPipelineInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(logicalDevice.device, VK_NULL_HANDLE, 1,
                                &graphicsPipelineInfo, nullptr,
                                &dataSystem.pipeline) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create graphics pipeline.");

    return RE_CRITICAL;
  }

  vkDestroyShaderModule(logicalDevice.device, vsModule, nullptr);
  vkDestroyShaderModule(logicalDevice.device, fsModule, nullptr);

  return RE_OK;
}

void MGraphics::destroyGraphicsPipeline() {
  RE_LOG(Log, "Shutting down graphics pipeline.");
  vkDestroyPipeline(logicalDevice.device, dataSystem.pipeline, nullptr);
  vkDestroyPipelineLayout(logicalDevice.device, dataSystem.pipelineLayout, nullptr);
}

TResult MGraphics::createCommandPool() {
  RE_LOG(Log, "Creating command pool.");

  VkCommandPoolCreateInfo commandPoolInfo{};
  commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  commandPoolInfo.queueFamilyIndex =
      physicalDevice.queueFamilyIndices.graphics[0];

  if (vkCreateCommandPool(logicalDevice.device, &commandPoolInfo, nullptr,
                          &dataSystem.commandPool) != VK_SUCCESS) {
    RE_LOG(Critical,
           "failed to create command pool for graphics queue family.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

void MGraphics::destroyCommandPool() {
  RE_LOG(Log, "Destroying command pool.");
  vkDestroyCommandPool(logicalDevice.device, dataSystem.commandPool, nullptr);
}

TResult MGraphics::createCommandBuffers() {
  RE_LOG(Log, "Creating rendering command buffers for %d frames.",
         MAX_FRAMES_IN_FLIGHT);

  dataSystem.cmdBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo cmdBufferInfo{};
  cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdBufferInfo.commandPool = dataSystem.commandPool;
  cmdBufferInfo.commandBufferCount =
      (uint32_t)dataSystem.cmdBuffers.size();

  if (vkAllocateCommandBuffers(logicalDevice.device, &cmdBufferInfo,
                               dataSystem.cmdBuffers.data()) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to allocate rendering command buffers.");
    return RE_CRITICAL;
  }

  RE_LOG(Log, "Creating aux command buffer.");

  cmdBufferInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(logicalDevice.device, &cmdBufferInfo,
                               &dataSystem.auxBuffer) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to allocate aux command buffer.");
    return RE_CRITICAL;
  }

  return RE_OK;
}

void MGraphics::destroyCommandBuffers() {
  RE_LOG(Log, "Freeing %d rendering command buffers.",
         dataSystem.cmdBuffers.size());
  vkFreeCommandBuffers(logicalDevice.device, dataSystem.commandPool,
                       static_cast<uint32_t>(dataSystem.cmdBuffers.size()),
                       dataSystem.cmdBuffers.data());

  RE_LOG(Log, "Freeing aux command buffer.");
  vkFreeCommandBuffers(logicalDevice.device, dataSystem.commandPool, 1,
                       &dataSystem.auxBuffer);
}

TResult MGraphics::recordCommandBuffer(VkCommandBuffer commandBuffer,
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
  renderPassInfo.renderPass = dataSystem.renderPass;
  renderPassInfo.framebuffer = dataSwapChain.framebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = dataSwapChain.imageExtent;
  renderPassInfo.pClearValues = &clearColor;
  renderPassInfo.clearValueCount = 1;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    dataSystem.pipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(dataSwapChain.imageExtent.width);
  viewport.height = static_cast<float>(dataSwapChain.imageExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = dataSwapChain.imageExtent;
  
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkBuffer buffers[] = {dataSystem.meshes[0]->vertexBuffer.buffer};
  VkDeviceSize offsets[] = {0};
  uint32_t numVertices = static_cast<uint32_t>(
      dataSystem.meshes[0]->vertexBuffer.allocInfo.size / sizeof(RVertex));

  vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

  vkCmdDraw(commandBuffer, numVertices, 1, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to end writing to command buffer.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

TResult MGraphics::createSyncObjects() {
  RE_LOG(Log, "Creating sychronization objects for %d frames.",
         MAX_FRAMES_IN_FLIGHT);

  dataSync.sImgAvailable.resize(MAX_FRAMES_IN_FLIGHT);
  dataSync.sRndrFinished.resize(MAX_FRAMES_IN_FLIGHT);
  dataSync.fInFlight.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semInfo{};
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenInfo{};
  fenInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // signaled to skip waiting for
                                                 // it on the first frame

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (vkCreateSemaphore(logicalDevice.device, &semInfo, nullptr,
                          &dataSync.sImgAvailable[i]) != VK_SUCCESS) {
      RE_LOG(Critical, "failed to create 'image available' semaphore.");

      return RE_CRITICAL;
    }

    if (vkCreateSemaphore(logicalDevice.device, &semInfo, nullptr,
                          &dataSync.sRndrFinished[i]) != VK_SUCCESS) {
      RE_LOG(Critical, "failed to create 'render finished' semaphore.");

      return RE_CRITICAL;
    }

    if (vkCreateFence(logicalDevice.device, &fenInfo, nullptr,
                      &dataSync.fInFlight[i])) {
      RE_LOG(Critical, "failed to create 'in flight' fence.");

      return RE_CRITICAL;
    }
  }

  return RE_OK;
}

void MGraphics::destroySyncObjects() {
  RE_LOG(Log, "Destroying synchronization objects.");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(logicalDevice.device, dataSync.sImgAvailable[i],
                       nullptr);
    vkDestroySemaphore(logicalDevice.device, dataSync.sRndrFinished[i],
                       nullptr);

    vkDestroyFence(logicalDevice.device, dataSync.fInFlight[i],
                   nullptr);
  }
}

TResult MGraphics::drawFrame() {
  uint32_t imageIndex = -1;
  TResult chkResult = RE_OK;

  vkWaitForFences(logicalDevice.device, 1,
                  &dataSync.fInFlight[dataSystem.idIFFrame], VK_TRUE,
                  UINT64_MAX);

  VkResult APIResult =
      vkAcquireNextImageKHR(logicalDevice.device, swapChain, UINT64_MAX,
                            dataSync.sImgAvailable[dataSystem.idIFFrame],
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

  // reset fences if we will do any work this frame e.g. no swap chain
  // recreation
  vkResetFences(logicalDevice.device, 1,
                &dataSync.fInFlight[dataSystem.idIFFrame]);

  vkResetCommandBuffer(dataSystem.cmdBuffers[dataSystem.idIFFrame], NULL);

  recordCommandBuffer(dataSystem.cmdBuffers[dataSystem.idIFFrame], imageIndex);

  // wait until image to write color data to is acquired
  VkSemaphore waitSems[] = {dataSync.sImgAvailable[dataSystem.idIFFrame]};
  VkSemaphore signalSems[] = {
      dataSync.sRndrFinished[dataSystem.idIFFrame]};
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
      &dataSystem.cmdBuffers[dataSystem.idIFFrame];  // submit command buffer
                                                   // recorded previously
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores =
      signalSems;  // signal these after rendering is finished

  // submit an array featuring command buffers to graphics queue and signal
  // fence for CPU to wait for execution
  if (vkQueueSubmit(logicalDevice.queues.graphics, 1, &submitInfo,
                    dataSync.fInFlight[dataSystem.idIFFrame]) !=
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

  dataSystem.idIFFrame = ++dataSystem.idIFFrame % MAX_FRAMES_IN_FLIGHT;

  return chkResult;
}