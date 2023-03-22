#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/renderer.h"

TResult core::MRenderer::createDescriptorSetLayouts() {
  // layout: shader binding / descriptor type / count / shader stage / immutable
  // samplers

  // scene matrices and environmental maps
  // 0 - MVP matrix
  // 1 - lighting variables
  // 2 - TODO
  // 3 - TODO
  // 4 - TODO
  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::Scene,
                                        VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
         nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    };
    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
        static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.at(EDescriptorSetLayout::Scene)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create scene descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  // standard set of 6 texture maps
  // 0 - baseColor
  // 1 - normal
  // 2 - metalness / roughness
  // 3 - ambient occlusion
  // 4 - emissive
  // 5 - extra
  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::Material,
                                        VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
        static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.at(EDescriptorSetLayout::Material)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create material descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  // model mesh matrices
  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::Mesh,
                                        VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
         nullptr}};

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
        static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.at(EDescriptorSetLayout::Mesh)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create model node descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  // environment map generation
  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::Environment,
                                        VK_NULL_HANDLE);

    // dynamic uniform buffer will contain 6 matrices for per-face
    // transformation
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
         VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
         nullptr}};

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
        static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.at(
                EDescriptorSetLayout::Environment)) != VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create environment descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  return RE_OK;
}

void core::MRenderer::destroyDescriptorSetLayouts() {
  RE_LOG(Log, "Removing descriptor set layouts.");

  for (auto& it : system.descriptorSetLayouts) {
    vkDestroyDescriptorSetLayout(logicalDevice.device, it.second, nullptr);
  }
}

TResult core::MRenderer::createDescriptorSets() {
#ifndef NDEBUG
  RE_LOG(Log, "Creating renderer descriptor sets.");
#endif

  {
    std::vector<VkDescriptorSetLayout> setLayouts(
        MAX_FRAMES_IN_FLIGHT,
        getDescriptorSetLayout(EDescriptorSetLayout::Scene));

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = system.descriptorPool;
    setAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    setAllocInfo.pSetLayouts = setLayouts.data();

    system.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateDescriptorSets(logicalDevice.device, &setAllocInfo,
                                 system.descriptorSets.data()) != VK_SUCCESS) {
      RE_LOG(Critical, "Failed to allocate descriptor sets.");
      return RE_CRITICAL;
    }

#ifndef NDEBUG
    RE_LOG(Log, "Populating renderer descriptor sets.");
#endif

    uint32_t descriptorCount = 2u;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      // model*view*projection data for descriptor set
      VkDescriptorBufferInfo descriptorBufferInfoMVP;
      descriptorBufferInfoMVP.buffer =
          view.modelViewProjectionBuffers[i].buffer;
      descriptorBufferInfoMVP.offset = 0;
      descriptorBufferInfoMVP.range = sizeof(RSceneUBO);

      // lighting data for descriptor set
      VkDescriptorBufferInfo descriptorBufferInfoLighting;
      descriptorBufferInfoLighting.buffer = lighting.buffers[i].buffer;
      descriptorBufferInfoLighting.offset = 0;
      descriptorBufferInfoLighting.range = sizeof(RLightingUBO);

      std::vector<VkWriteDescriptorSet> writeDescriptorSets(descriptorCount);

      // settings used for writing to MVP descriptor set
      writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeDescriptorSets[0].dstSet = system.descriptorSets[i];
      writeDescriptorSets[0].dstBinding = 0;
      writeDescriptorSets[0].dstArrayElement = 0;
      writeDescriptorSets[0].descriptorCount = 1;
      writeDescriptorSets[0].pBufferInfo = &descriptorBufferInfoMVP;
      writeDescriptorSets[0].pImageInfo = nullptr;
      writeDescriptorSets[0].pTexelBufferView = nullptr;
      writeDescriptorSets[0].pNext = nullptr;

      // settings used for writing to lighting descriptor set
      writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeDescriptorSets[1].descriptorCount = 1;
      writeDescriptorSets[1].dstSet = system.descriptorSets[i];
      writeDescriptorSets[1].dstBinding = 1;
      writeDescriptorSets[1].pBufferInfo = &descriptorBufferInfoLighting;
      /*
      // environment image data
      writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[2].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[2].descriptorCount = 1;
      writeDescriptorSets[2].dstSet = system.descriptorSets[i];
      writeDescriptorSets[2].dstBinding = 2;
      writeDescriptorSets[2].pImageInfo = &textures.irradianceCube.descriptor;

      writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[3].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[3].descriptorCount = 1;
      writeDescriptorSets[3].dstSet = system.descriptorSets[i];
      writeDescriptorSets[3].dstBinding = 3;
      writeDescriptorSets[3].pImageInfo = &textures.prefilteredCube.descriptor;

      writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[4].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[4].descriptorCount = 1;
      writeDescriptorSets[4].dstSet = system.descriptorSets[i];
      writeDescriptorSets[4].dstBinding = 4;
      writeDescriptorSets[4].pImageInfo = &textures.lutBrdf.descriptor;
      */
      vkUpdateDescriptorSets(logicalDevice.device, descriptorCount,
                             writeDescriptorSets.data(), 0, nullptr);
    }
  }

#ifndef NDEBUG
  RE_LOG(Log, "Creating environment desriptor set.");
#endif

  {
    std::vector<VkDescriptorSetLayout> environmentSetLayout(
        MAX_FRAMES_IN_FLIGHT,
        getDescriptorSetLayout(EDescriptorSetLayout::Environment));

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = system.descriptorPool;
    setAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    setAllocInfo.pSetLayouts = environmentSetLayout.data();

    environment.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateDescriptorSets(logicalDevice.device, &setAllocInfo,
                                 environment.descriptorSets.data()) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to allocate descriptor sets.");
      return RE_CRITICAL;
    }

#ifndef NDEBUG
    RE_LOG(Log, "Populating environment descriptor set.");
#endif

    for (uint32_t j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j) {
      VkDescriptorBufferInfo infoEnvironment;
      infoEnvironment.buffer = environment.transformBuffers[j].buffer;
      infoEnvironment.offset = 0;
      infoEnvironment.range = sizeof(REnvironmentUBO);

      // lighting data for descriptor set
      VkDescriptorBufferInfo infoLighting;
      infoLighting.buffer = lighting.buffers[j].buffer;
      infoLighting.offset = 0;
      infoLighting.range = sizeof(RLightingUBO);

      std::vector<VkWriteDescriptorSet> writeSets(2);
      writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      writeSets[0].dstSet = environment.descriptorSets[j];
      writeSets[0].dstBinding = 0;
      writeSets[0].dstArrayElement = 0;
      writeSets[0].descriptorCount = 1;
      writeSets[0].pBufferInfo = &infoEnvironment;

      writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeSets[1].dstSet = environment.descriptorSets[j];
      writeSets[1].dstBinding = 1;
      writeSets[1].descriptorCount = 1;
      writeSets[1].pBufferInfo = &infoLighting;

      vkUpdateDescriptorSets(logicalDevice.device,
                             static_cast<uint32_t>(writeSets.size()),
                             writeSets.data(), 0, nullptr);
    }
  }

  return RE_OK;
}

TResult core::MRenderer::createDefaultFramebuffers() {
  RE_LOG(Log, "Creating swapchain framebuffers.");

  swapchain.framebuffers.resize(swapchain.imageViews.size());

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

  for (size_t i = 0; i < swapchain.framebuffers.size(); ++i) {
    std::vector<VkImageView> attachments = {
        swapchain.imageViews[i],
        core::resources.getTexture(RT_DEPTH)->texture.view};

    framebufferInfo.renderPass = getVkRenderPass(ERenderPass::PBR);
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapchain.imageExtent.width;
    framebufferInfo.height = swapchain.imageExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(logicalDevice.device, &framebufferInfo, nullptr,
                            &swapchain.framebuffers[i]) != VK_SUCCESS) {
      RE_LOG(Critical, "failed to create swapchain framebuffer with index %d.",
             i);

      return RE_CRITICAL;
    }
  }

  return createFramebuffer(ERenderPass::Environment, RT_FRONT, FB_FRONT);
}

TResult core::MRenderer::createFramebuffer(ERenderPass renderPass,
                                           const char* targetTextureName,
                                           const char* framebufferName) {
  // create render-to-texture framebuffer
  RTexture* fbTarget = core::resources.getTexture(targetTextureName);

  if (!fbTarget) {
    RE_LOG(Error,
           "Failed to create framebuffer %s, target texture %s wasn't found.",
           framebufferName, targetTextureName);
    return RE_ERROR;
  }

  std::string fbName = framebufferName;

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = getVkRenderPass(renderPass);
  framebufferInfo.attachmentCount = 1;
  framebufferInfo.pAttachments = &fbTarget->texture.view;
  framebufferInfo.width = fbTarget->texture.width;
  framebufferInfo.height = fbTarget->texture.height;
  framebufferInfo.layers = fbTarget->texture.layerCount;

  if (!system.framebuffers.try_emplace(fbName).second) {
#ifndef NDEBUG
    RE_LOG(Warning,
           "Failed to create framebuffer record for \"%s\". Already exists.",
           fbName.c_str());
#endif
    return RE_WARNING;
  }

  if (vkCreateFramebuffer(logicalDevice.device, &framebufferInfo, nullptr,
                          &system.framebuffers.at(fbName)) != VK_SUCCESS) {
    RE_LOG(Error, "failed to create framebuffer %s.", fbName.c_str());

    return RE_ERROR;
  }

  return RE_OK;
}

TResult core::MRenderer::createRenderPasses() {
  //
  // MAIN render pass
  //
  ERenderPass passType = ERenderPass::PBR;

  RE_LOG(Log, "Creating render pass E%d", passType);

  system.renderPasses.emplace(passType, RRenderPass{});

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapchain.formatData.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp =
      VK_ATTACHMENT_LOAD_OP_CLEAR;  // clearing contents on new frame
  colorAttachment.storeOp =
      VK_ATTACHMENT_STORE_OP_STORE;  // storing contents in memory while
                                     // rendering
  colorAttachment.stencilLoadOp =
      VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // ignore stencil buffer
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout =
      VK_IMAGE_LAYOUT_UNDEFINED;  // not important, since it's cleared at the
                                  // frame start
  colorAttachment.finalLayout =
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // swap chain image to be presented

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment =
      0;  // index of an attachment in attachmentdesc array (also vertex shader
          // output index)
  colorAttachmentRef.layout =
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // the layout will be an image

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = core::vulkan::formatDepth;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpassDesc{};
  subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDesc.colorAttachmentCount = 1;
  subpassDesc.pColorAttachments = &colorAttachmentRef;
  subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependency{};  // makes subpass wait for color
                                     // attachment-type image to be acquired
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;  // wait for external subpass
                                                   // to be of color attachment
  dependency.srcAccessMask = NULL;
  dependency.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;  // wait until we can write to
                                                   // color attachment
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::vector<VkAttachmentDescription> attachments = {colorAttachment,
                                                      depthAttachment};

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDesc;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(logicalDevice.device, &renderPassInfo, nullptr,
                         &getVkRenderPass(passType)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create render pass E%d.", passType);

    return RE_CRITICAL;
  }

  //
  // CUBEMAP render pass
  //
  passType = ERenderPass::Environment;
  attachments.clear();

  const uint32_t numMips =
      static_cast<uint32_t>(floor(log2(core::vulkan::envFilterExtent))) + 1;

  system.renderPasses.emplace(passType, RRenderPass{});

  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.format = core::vulkan::formatHDR16;
  subpassDesc.pDepthStencilAttachment = nullptr;
  attachments = {colorAttachment};
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();

  if (vkCreateRenderPass(logicalDevice.device, &renderPassInfo, nullptr,
                         &getVkRenderPass(passType)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create render pass E%d.", passType);

    return RE_CRITICAL;
  }

  // setup default render pass begin info
  system.clearColors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
  system.clearColors[1].depthStencil = {1.0f, 0};

  system.renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  system.renderPassBeginInfo.renderArea.offset = {0, 0};
  system.renderPassBeginInfo.renderArea.extent = swapchain.imageExtent;
  system.renderPassBeginInfo.pClearValues = system.clearColors.data();
  system.renderPassBeginInfo.clearValueCount =
      static_cast<uint32_t>(system.clearColors.size());

  // must be set up during frame drawing
  system.renderPassBeginInfo.renderPass = VK_NULL_HANDLE;
  system.renderPassBeginInfo.framebuffer = VK_NULL_HANDLE;

  return RE_OK;
}

void core::MRenderer::destroyRenderPasses() {
  RE_LOG(Log, "Destroying render passes.");

  for (auto& it : system.renderPasses) {
    vkDestroyRenderPass(logicalDevice.device, it.second.renderPass, nullptr);
  }
}

RRenderPass* core::MRenderer::getRenderPass(ERenderPass type) {
  if (system.renderPasses.contains(type)) {
    return &system.renderPasses.at(type);
  }

  RE_LOG(Error, "Failed to get render pass E%d, it does not exist.", type);
  return VK_NULL_HANDLE;
}

VkRenderPass& core::MRenderer::getVkRenderPass(ERenderPass type) {
  return system.renderPasses.at(type).renderPass;
}

TResult core::MRenderer::createGraphicsPipelines() {
  RE_LOG(Log, "Creating graphics pipelines.");

  //
  // PIPELINE LAYOUTS
  //

  // common 3D pipeline data //

#ifndef NDEBUG
  RE_LOG(Log, "Setting up common 3D pipeline data.");
#endif

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  inputAssemblyInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  // set main viewport
  VkViewport& viewport = getRenderPass(ERenderPass::PBR)->viewport;
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

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchain.imageExtent;

  VkPipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportInfo.pViewports = &viewport;
  viewportInfo.viewportCount = 1;
  viewportInfo.pScissors = &scissor;
  viewportInfo.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
  rasterizationInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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
  depthStencilInfo.depthWriteEnable = VK_TRUE;
  depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilInfo.front = depthStencilInfo.back;
  depthStencilInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
  depthStencilInfo.stencilTestEnable = VK_FALSE;
  depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  depthStencilInfo.minDepthBounds = 0.0f;
  depthStencilInfo.maxDepthBounds = 1.0f;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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

  // dynamic states allow changing specific parts of the pipeline without
  // recreating it
  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynStateInfo{};
  dynStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynStateInfo.pDynamicStates = dynamicStates.data();

  VkPushConstantRange materialPushConstRange{};
  materialPushConstRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  materialPushConstRange.offset = 0;
  materialPushConstRange.size = sizeof(RMaterialPCB);

#ifndef NDEBUG
  RE_LOG(Log, "Creating pipeline layout for \"scene\".");
#endif

  EPipelineLayout layoutType = EPipelineLayout::Scene;
  system.layouts.emplace(layoutType, VK_NULL_HANDLE);

  // pipeline layout for the main 'scene'
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
      getDescriptorSetLayout(EDescriptorSetLayout::Scene),    // 0
      getDescriptorSetLayout(EDescriptorSetLayout::Mesh),     // 1
      getDescriptorSetLayout(EDescriptorSetLayout::Material)  // 2
  };

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  layoutInfo.pSetLayouts = descriptorSetLayouts.data();
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &materialPushConstRange;

  if (vkCreatePipelineLayout(logicalDevice.device, &layoutInfo, nullptr,
                             &getPipelineLayout(layoutType)) != VK_SUCCESS) {
    RE_LOG(Critical, "failed to create \"scene\" pipeline layout.");

    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Creating pipeline layout for \"environment\".");
#endif

  // pipeline layout for the 'environment' map generation
  layoutType = EPipelineLayout::Environment;
  system.layouts.emplace(layoutType, VK_NULL_HANDLE);

  descriptorSetLayouts.clear();
  descriptorSetLayouts = {
      getDescriptorSetLayout(EDescriptorSetLayout::Environment),  // 0
      getDescriptorSetLayout(EDescriptorSetLayout::Mesh),         // 1
      getDescriptorSetLayout(EDescriptorSetLayout::Material)      // 2
  };

  VkPushConstantRange envPushConstRange{};
  envPushConstRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  envPushConstRange.offset = 0;
  envPushConstRange.size = sizeof(REnvironmentPCB);

  layoutInfo = VkPipelineLayoutCreateInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  layoutInfo.pSetLayouts = descriptorSetLayouts.data();
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &envPushConstRange;

  if (vkCreatePipelineLayout(logicalDevice.device, &layoutInfo, nullptr,
                             &getPipelineLayout(layoutType)) != VK_SUCCESS) {
    RE_LOG(Critical, "failed to create \"environment\" pipeline layout.");

    return RE_CRITICAL;
  }

  //
  // PIPELINES
  //

  VkVertexInputBindingDescription bindingDesc = RVertex::getBindingDesc();
  auto attributeDescs = RVertex::getAttributeDescs();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescs.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

  // 'PBR single sided' pipeline, uses 'PBR' render pass and 'scene' layout
#ifndef NDEBUG
  RE_LOG(Log, "Setting up PBR pipeline.");
#endif

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
      loadShader("vs_pbr.spv", VK_SHADER_STAGE_VERTEX_BIT),
      loadShader("fs_pbr.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
  graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  graphicsPipelineInfo.pColorBlendState = &colorBlendInfo;
  graphicsPipelineInfo.pDepthStencilState = &depthStencilInfo;
  graphicsPipelineInfo.pDynamicState = &dynStateInfo;
  graphicsPipelineInfo.pMultisampleState = &multisampleInfo;
  graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
  graphicsPipelineInfo.pRasterizationState = &rasterizationInfo;
  graphicsPipelineInfo.pViewportState = &viewportInfo;
  graphicsPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  graphicsPipelineInfo.pStages = shaderStages.data();
  graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
  graphicsPipelineInfo.layout = getPipelineLayout(EPipelineLayout::Scene);
  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::PBR);
  graphicsPipelineInfo.subpass = 0;  // subpass index for render pass
  graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  graphicsPipelineInfo.basePipelineIndex = -1;

  system.pipelines.emplace(EPipeline::OpaqueCullBack, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(logicalDevice.device, VK_NULL_HANDLE, 1,
                                &graphicsPipelineInfo, nullptr,
                                &getPipeline(OpaqueCullBack)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create PBR graphics pipeline.");

    return RE_CRITICAL;
  }

  // 'PBR doublesided' pipeline, uses 'PBR' render pass and 'scene' layout
  rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

  system.pipelines.emplace(EPipeline::OpaqueCullNone, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(logicalDevice.device, VK_NULL_HANDLE, 1,
                                &graphicsPipelineInfo, nullptr,
                                &getPipeline(OpaqueCullNone)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create PBR doublesided graphics pipeline.");

    return RE_CRITICAL;
  }

  for (auto stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  // 'Skybox' pipeline
#ifndef NDEBUG
  RE_LOG(Log, "Setting up the skybox pipeline.");
#endif

  shaderStages.clear();
  shaderStages = {loadShader("vs_skybox.spv", VK_SHADER_STAGE_VERTEX_BIT),
                  loadShader("fs_skybox.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;

  system.pipelines.emplace(EPipeline::Skybox, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(logicalDevice.device, VK_NULL_HANDLE, 1,
                                &graphicsPipelineInfo, nullptr,
                                &getPipeline(Skybox)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create skybox graphics pipeline.");

    return RE_CRITICAL;
  }

  for (auto stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  shaderStages.clear();
  shaderStages = {loadShader("vs_environment.spv", VK_SHADER_STAGE_VERTEX_BIT),
                  loadShader("fs_envFilter.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  system.pipelines.emplace(EPipeline::EnvFilter, VK_NULL_HANDLE);

  graphicsPipelineInfo.layout = getPipelineLayout(EPipelineLayout::Environment);
  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::Environment);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::EnvFilter)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create environment filter graphics pipeline.");

    return RE_CRITICAL;
  }

  for (auto stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  shaderStages.clear();
  shaderStages = {loadShader("vs_environment.spv", VK_SHADER_STAGE_VERTEX_BIT),
                  loadShader("fs_envIrradiance.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  system.pipelines.emplace(EPipeline::EnvIrradiance, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::EnvIrradiance)) != VK_SUCCESS) {
    RE_LOG(Critical,
           "Failed to create environment irradiance graphics pipeline.");

    return RE_CRITICAL;
  }

  for (auto stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  return RE_OK;
}

void core::MRenderer::destroyGraphicsPipelines() {
  RE_LOG(Log, "Shutting down graphics pipeline.");
  destroyDescriptorSetLayouts();

  for (auto& pipeline : system.pipelines) {
    vkDestroyPipeline(logicalDevice.device, pipeline.second, nullptr);
  }

  for (auto& layout : system.layouts) {
    vkDestroyPipelineLayout(logicalDevice.device, layout.second, nullptr);
  }
}

TResult core::MRenderer::configureRenderPasses() {
  // configure main 'scene', PBR render pass
  RRenderPass* pRenderPass = getRenderPass(ERenderPass::PBR);
  pRenderPass->usedLayout = getPipelineLayout(EPipelineLayout::Scene);
  pRenderPass->usedPipelines.emplace_back(EPipeline::OpaqueCullBack);
  pRenderPass->usedPipelines.emplace_back(EPipeline::OpaqueCullNone);
  pRenderPass->usedPipelines.emplace_back(EPipeline::BlendCullBack);
  pRenderPass->usedPipelines.emplace_back(EPipeline::Skybox);

  // configure 'environment' cubemap render pass
  pRenderPass = getRenderPass(ERenderPass::Environment);
  pRenderPass->usedLayout = getPipelineLayout(EPipelineLayout::Environment);
  pRenderPass->usedPipelines.emplace_back(EPipeline::EnvFilter);
  pRenderPass->usedPipelines.emplace_back(EPipeline::EnvIrradiance);
  pRenderPass->viewport.width =
      static_cast<float>(core::vulkan::envFilterExtent);
  pRenderPass->viewport.height = core::vulkan::bFlipViewPortY
                                     ? -pRenderPass->viewport.width
                                     : pRenderPass->viewport.width;
  pRenderPass->viewport.y =
      core::vulkan::bFlipViewPortY ? pRenderPass->viewport.width : 0;

  return RE_OK;
}