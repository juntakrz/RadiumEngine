#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/renderer.h"

TResult core::MRenderer::createDescriptorSetLayouts() {
  // layout: shader binding / descriptor type / count / shader stage / immutable
  // samplers

  // dummy descriptor set layout
  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::Dummy,
                                        VK_NULL_HANDLE);

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.at(EDescriptorSetLayout::Dummy)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create dummy descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  // scene matrices and environmental maps
  // 0 - MVP matrix
  // 1 - lighting variables
  // 2 - environment filtered map
  // 3 - environment irradiance map
  // 4 - generated BRDF LUT map
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

  // model transform matrices
  // 0 - model transform matrices (offset stored in entity)
  // 1 - per node transform matrices (offset SHOULD BE stored by animations manager)
  // 2 - per model inverse bind matrices (offset SHOULD BE stored by animations manager)
  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::Model,
                                        VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
         VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
         VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
         VK_SHADER_STAGE_VERTEX_BIT, nullptr}};

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
        static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.at(EDescriptorSetLayout::Model)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create model descriptor set layout.");
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

    uint32_t descriptorCount = 5u;

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
      
      // environment image data

      // environment maps are created with a layout for accepting data writes
      // however descriptor sets require info about their final state
      VkDescriptorImageInfo imageDescriptors[3]{
          core::resources.getTexture(RTGT_ENVFILTER)->texture.descriptor,
          core::resources.getTexture(RTGT_ENVIRRAD)->texture.descriptor,
          core::resources.getTexture(RTGT_LUTMAP)->texture.descriptor};

      for (VkDescriptorImageInfo& imageInfo : imageDescriptors) {
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }

      // RTGT_ENVFILTER
      writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[2].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[2].descriptorCount = 1;
      writeDescriptorSets[2].dstSet = system.descriptorSets[i];
      writeDescriptorSets[2].dstBinding = 2;
      writeDescriptorSets[2].pImageInfo = &imageDescriptors[0];

      // RTGT_ENVIRRAD
      writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[3].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[3].descriptorCount = 1;
      writeDescriptorSets[3].dstSet = system.descriptorSets[i];
      writeDescriptorSets[3].dstBinding = 3;
      writeDescriptorSets[3].pImageInfo = &imageDescriptors[1];

      // RTGT_LUTMAP
      writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[4].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[4].descriptorCount = 1;
      writeDescriptorSets[4].dstSet = system.descriptorSets[i];
      writeDescriptorSets[4].dstBinding = 4;
      writeDescriptorSets[4].pImageInfo = &imageDescriptors[2];
      
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

    environment.envDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateDescriptorSets(logicalDevice.device, &setAllocInfo,
                                 environment.envDescriptorSets.data()) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to allocate descriptor sets.");
      return RE_CRITICAL;
    }

#ifndef NDEBUG
    RE_LOG(Log, "Populating environment descriptor set.");
#endif

    for (uint32_t j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j) {
      VkDescriptorBufferInfo infoEnvironment{};
      infoEnvironment.buffer = environment.transformBuffers[j].buffer;
      infoEnvironment.offset = 0;
      infoEnvironment.range = sizeof(REnvironmentUBO);

      // lighting data for descriptor set
      VkDescriptorBufferInfo infoLighting{};
      infoLighting.buffer = lighting.buffers[j].buffer;
      infoLighting.offset = 0;
      infoLighting.range = sizeof(RLightingUBO);

      std::vector<VkWriteDescriptorSet> writeSets(2);
      writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      writeSets[0].dstSet = environment.envDescriptorSets[j];
      writeSets[0].dstBinding = 0;
      writeSets[0].dstArrayElement = 0;
      writeSets[0].descriptorCount = 1;
      writeSets[0].pBufferInfo = &infoEnvironment;

      writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeSets[1].dstSet = environment.envDescriptorSets[j];
      writeSets[1].dstBinding = 1;
      writeSets[1].descriptorCount = 1;
      writeSets[1].pBufferInfo = &infoLighting;

      vkUpdateDescriptorSets(logicalDevice.device,
                             static_cast<uint32_t>(writeSets.size()),
                             writeSets.data(), 0, nullptr);
    }
  }

#ifndef NDEBUG
  RE_LOG(Log, "Creating model transformation descriptor set.");
#endif

  {
    VkDevice device = core::renderer.logicalDevice.device;

    VkDescriptorSetLayout meshLayout =
        getDescriptorSetLayout(EDescriptorSetLayout::Model);

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = core::renderer.getDescriptorPool();
    descriptorSetAllocInfo.pSetLayouts = &meshLayout;
    descriptorSetAllocInfo.descriptorSetCount = 1;

    VkResult result;
    if ((result = vkAllocateDescriptorSets(
             device, &descriptorSetAllocInfo,
             &scene.transformSet)) != VK_SUCCESS) {
      RE_LOG(Error,
             "Failed to create model transformation descriptor set. Vulkan "
             "error %d.",
             result);
      return RE_ERROR;
    }

    #ifndef NDEBUG
    RE_LOG(Log, "Populating model transformation descriptor set.");
#endif

    VkDescriptorBufferInfo rootMatrixBufferInfo{};
    rootMatrixBufferInfo.buffer = scene.rootTransformBuffer.buffer;
    rootMatrixBufferInfo.offset = 0;
    rootMatrixBufferInfo.range = sizeof(glm::mat4); // root matrix

    VkDescriptorBufferInfo nodeMatrixBufferInfo{};
    nodeMatrixBufferInfo.buffer = scene.nodeTransformBuffer.buffer;
    nodeMatrixBufferInfo.offset = 0;
    nodeMatrixBufferInfo.range = RE_NODEDATASIZE;

    VkDescriptorBufferInfo skinningMatricesBufferInfo{};
    skinningMatricesBufferInfo.buffer = scene.skinTransformBuffer.buffer;
    skinningMatricesBufferInfo.offset = 0;
    skinningMatricesBufferInfo.range = RE_SKINDATASIZE;

    std::vector<VkWriteDescriptorSet> writeSets(3);
    writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writeSets[0].descriptorCount = 1;
    writeSets[0].dstSet = scene.transformSet;
    writeSets[0].dstBinding = 0;
    writeSets[0].pBufferInfo = &rootMatrixBufferInfo;

    writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writeSets[1].descriptorCount = 1;
    writeSets[1].dstSet = scene.transformSet;
    writeSets[1].dstBinding = 1;
    writeSets[1].pBufferInfo = &nodeMatrixBufferInfo;

    writeSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writeSets[2].descriptorCount = 1;
    writeSets[2].dstSet = scene.transformSet;
    writeSets[2].dstBinding = 2;
    writeSets[2].pBufferInfo = &skinningMatricesBufferInfo;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()),
                           writeSets.data(), 0, nullptr);
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
        core::resources.getTexture(RTGT_DEPTH)->texture.view};

    framebufferInfo.renderPass = getVkRenderPass(ERenderPass::Present);
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

  TResult chkResult;
  std::vector<std::string> attachmentNames;
  attachmentNames.emplace_back(RTGT_GPOSITION); // 0
  attachmentNames.emplace_back(RTGT_GDIFFUSE);  // 1
  attachmentNames.emplace_back(RTGT_GNORMAL);   // 2
  attachmentNames.emplace_back(RTGT_GPHYSICAL); // 3
  attachmentNames.emplace_back(RTGT_GEMISSIVE); // 4
  attachmentNames.emplace_back(RTGT_DEPTH);     // depth buffer target

  if (chkResult = createFramebuffer(ERenderPass::Deferred, attachmentNames,
                                    RFB_DEFERRED) != RE_OK) {
    return chkResult;
  }

  attachmentNames.clear();
  attachmentNames.emplace_back(RTGT_GPBR);      // 0 - combine G-buffer textures
  attachmentNames.emplace_back(RTGT_DEPTH);     // depth buffer target

  if (chkResult = createFramebuffer(ERenderPass::PBR, attachmentNames,
                                    RFB_PBR) != RE_OK) {
    return chkResult;
  }

  // framebuffer for rendering environment map source
  if (chkResult = createFramebuffer(ERenderPass::Environment, {RTGT_ENVSRC},
                                    RFB_ENV) != RE_OK) {
    return chkResult;
  };

  // framebuffer for generating and storing LUT map
  return createFramebuffer(ERenderPass::LUTGen, {RTGT_LUTMAP}, RFB_LUT);
}

TResult core::MRenderer::createRenderPasses() {
  // lambda for quickly filling common clear values for render targets
  auto fGetClearValues = [](VkClearColorValue colorValue,
                            int8_t colorValueCount,
                            VkClearDepthStencilValue depthValue) {
    std::vector<VkClearValue> clearValues;
    clearValues.resize(colorValueCount + 1);

    for (int8_t i = 0; i < colorValueCount; ++i) {
      clearValues[i].color = colorValue;
    }

    clearValues[colorValueCount].depthStencil = depthValue;
    return clearValues;
  };
  
  // standard info setup
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = core::vulkan::formatHDR16;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp =
      VK_ATTACHMENT_LOAD_OP_CLEAR;  // clearing contents on new frame
  colorAttachment.storeOp =
      VK_ATTACHMENT_STORE_OP_STORE; // storing contents in memory while rendering
  colorAttachment.stencilLoadOp =
      VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // ignore stencil buffer
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout =
      VK_IMAGE_LAYOUT_UNDEFINED;    // not important, since it's cleared at the
                                    // frame start
  colorAttachment.finalLayout =
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // swap chain image to be presented

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

  // common clear color and clear depth values
  const VkClearColorValue clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
  const VkClearDepthStencilValue clearDepth = {1.0f};

  //
  // DEFERRED render pass
  // all models are rendered to 5 separate color maps forming G-buffer

  ERenderPass passType = ERenderPass::Deferred;
  system.renderPasses.emplace(passType, RRenderPass{});
  RE_LOG(Log, "Creating render pass E%d", passType);

  colorAttachment.format = core::vulkan::formatHDR16;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // deferred render targets: position, color, normal, physical, emissive
  VkAttachmentDescription deferredColorAttachments[] = {
      colorAttachment, colorAttachment, colorAttachment, colorAttachment,
      colorAttachment};

  VkRenderPass newRenderPass = createRenderPass(
      logicalDevice.device, 5, deferredColorAttachments, &depthAttachment);
  system.renderPasses.at(passType).renderPass = newRenderPass;
  system.renderPasses.at(passType).clearValues =
      fGetClearValues(clearColor, 5, clearDepth);

  //
  // PBR render pass
  // textures from deferred pass are used as a single material
  passType = ERenderPass::PBR;
  system.renderPasses.emplace(passType, RRenderPass{});
  RE_LOG(Log, "Creating render pass E%d", passType);

  newRenderPass = createRenderPass(
      logicalDevice.device, 1, &colorAttachment, &depthAttachment);
  system.renderPasses.at(passType).renderPass = newRenderPass;
  system.renderPasses.at(passType).clearValues =
      fGetClearValues(clearColor, 1, clearDepth);

  //
  // PRESENT render pass
  // the 16 bit float color map is rendered to the compatible surface target
  passType = ERenderPass::Present;
  system.renderPasses.emplace(passType, RRenderPass{});
  RE_LOG(Log, "Creating render pass E%d", passType);

  colorAttachment.format = swapchain.formatData.format;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  newRenderPass = createRenderPass(logicalDevice.device, 1, &colorAttachment,
                                   &depthAttachment);
  system.renderPasses.at(passType).renderPass = newRenderPass;
  system.renderPasses.at(passType).clearValues =
      fGetClearValues(clearColor, 1, clearDepth);

  //
  // CUBEMAP render pass
  //

  passType = ERenderPass::Environment;
  system.renderPasses.emplace(passType, RRenderPass{});
  RE_LOG(Log, "Creating render pass E%d", passType);

  colorAttachment.format = core::vulkan::formatHDR16;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  newRenderPass =
      createRenderPass(logicalDevice.device, 1, &colorAttachment, nullptr);
  system.renderPasses.at(passType).renderPass = newRenderPass;
  system.renderPasses.at(passType).clearValues =
      fGetClearValues(clearColor, 1, clearDepth);

  //
  // BRDF LUT render pass
  //
  passType = ERenderPass::LUTGen;
  system.renderPasses.emplace(passType, RRenderPass{});
  RE_LOG(Log, "Creating render pass E%d", passType);

  colorAttachment.format = core::vulkan::formatLUT;
  newRenderPass =
      createRenderPass(logicalDevice.device, 1, &colorAttachment, nullptr);
  system.renderPasses.at(passType).renderPass = newRenderPass;
  system.renderPasses.at(passType).clearValues =
      fGetClearValues(clearColor, 1, clearDepth);

  //
  // Shadow render pass
  //
  passType = ERenderPass::Shadow;
  system.renderPasses.emplace(passType, RRenderPass{});
  RE_LOG(Log, "Creating render pass E%d", passType);

  newRenderPass =
      createRenderPass(logicalDevice.device, 0, nullptr, &depthAttachment);
  system.renderPasses.at(passType).renderPass = newRenderPass;

  system.renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  system.renderPassBeginInfo.renderArea.offset = {0, 0};
  system.renderPassBeginInfo.renderArea.extent = swapchain.imageExtent;

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

  // set viewports and scissors for screen render passes
  VkViewport viewport{}; 
  viewport.x = 0.0f;
  viewport.y = static_cast<float>(swapchain.imageExtent.height);
  viewport.width = static_cast<float>(swapchain.imageExtent.width);
  viewport.height = -viewport.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  getRenderPass(ERenderPass::Deferred)->viewport = viewport;
  getRenderPass(ERenderPass::PBR)->viewport = viewport;
  getRenderPass(ERenderPass::Present)->viewport = viewport;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchain.imageExtent;

  getRenderPass(ERenderPass::Deferred)->scissor = scissor;
  getRenderPass(ERenderPass::PBR)->scissor = scissor;
  getRenderPass(ERenderPass::Present)->scissor = scissor;

  VkPipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportInfo.pViewports = &getRenderPass(ERenderPass::PBR)->viewport;
  viewportInfo.viewportCount = 1;
  viewportInfo.pScissors = &getRenderPass(ERenderPass::PBR)->scissor;
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
  colorBlendAttachment.blendEnable = VK_FALSE;
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
      getDescriptorSetLayout(EDescriptorSetLayout::Scene),      // 0
      getDescriptorSetLayout(EDescriptorSetLayout::Model),      // 1
      getDescriptorSetLayout(EDescriptorSetLayout::Material)    // 2
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
      getDescriptorSetLayout(EDescriptorSetLayout::Model),        // 1
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

  // pipeline layout for generating BRDF LUT map
  layoutType = EPipelineLayout::LUTGen;
  system.layouts.emplace(layoutType, VK_NULL_HANDLE);

  descriptorSetLayouts.clear();
  descriptorSetLayouts = {getDescriptorSetLayout(EDescriptorSetLayout::Dummy)};

  layoutInfo = VkPipelineLayoutCreateInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  layoutInfo.pSetLayouts = descriptorSetLayouts.data();
  layoutInfo.pushConstantRangeCount = 0;
  layoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(logicalDevice.device, &layoutInfo, nullptr,
                             &getPipelineLayout(layoutType)) != VK_SUCCESS) {
    RE_LOG(Critical, "failed to create \"BRDF LUT generator\" pipeline layout.");

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

  // 'single sided' pipeline, uses 'Deferred' render pass and 'scene' layout
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
      loadShader("vs_deferred.spv", VK_SHADER_STAGE_VERTEX_BIT),
      loadShader("fs_deferred.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  VkPipelineColorBlendAttachmentState colorBlendAttachments[] = {
      colorBlendAttachment, colorBlendAttachment, colorBlendAttachment,
      colorBlendAttachment, colorBlendAttachment};

  colorBlendInfo.attachmentCount = 5;
  colorBlendInfo.pAttachments = colorBlendAttachments;

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
  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::Deferred);
  graphicsPipelineInfo.subpass = 0;  // subpass index for render pass
  graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  graphicsPipelineInfo.basePipelineIndex = -1;

  system.pipelines.emplace(EPipeline::OpaqueCullBack, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::OpaqueCullBack)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create PBR graphics pipeline.");

    return RE_CRITICAL;
  }

  // 'Doublesided' pipeline, uses 'Deferred' render pass and 'scene' layout
  rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

  system.pipelines.emplace(EPipeline::OpaqueCullNone, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::OpaqueCullNone)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create PBR doublesided graphics pipeline.");

    return RE_CRITICAL;
  }

  // 'Blend' pipeline, similar to singlesided, but with alpha channel
  rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
  colorBlendAttachment.blendEnable = VK_TRUE;

  system.pipelines.emplace(EPipeline::BlendCullNone, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::BlendCullNone)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create PBR doublesided graphics pipeline.");

    return RE_CRITICAL;
  }

  for (auto stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  // 'PBR' pipeline, renders combined G-buffer output to a screen wide plane
  shaderStages.clear();
  shaderStages = {loadShader("vs_pbr.spv", VK_SHADER_STAGE_VERTEX_BIT),
                  loadShader("fs_pbr.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::PBR);

  rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;

  colorBlendInfo.attachmentCount = 1;
  colorBlendInfo.pAttachments = &colorBlendAttachment;

  system.pipelines.emplace(EPipeline::PBRDeferred, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::PBRDeferred)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create PBR graphics pipeline.");

    return RE_CRITICAL;
  }

  for (auto stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  // 'Skybox' pipeline
  shaderStages.clear();
  shaderStages = {loadShader("vs_skybox.spv", VK_SHADER_STAGE_VERTEX_BIT),
                  loadShader("fs_skybox.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;

  system.pipelines.emplace(EPipeline::Skybox, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::Skybox)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create skybox graphics pipeline.");

    return RE_CRITICAL;
  }

  for (auto stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  // 'Present' final output pipeline
  shaderStages.clear();
  shaderStages = {loadShader("vs_present.spv", VK_SHADER_STAGE_VERTEX_BIT),
                  loadShader("fs_present.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  system.pipelines.emplace(EPipeline::Present, VK_NULL_HANDLE);

  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::Present);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::Present)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create Vulkan presentation pipeline.");

    return RE_CRITICAL;
  }

  for (auto& stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  // 'EnvFilter' pipeline
  shaderStages.clear();
  shaderStages = {loadShader("vs_environment.spv", VK_SHADER_STAGE_VERTEX_BIT),
                  loadShader("fs_envFilter.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  system.pipelines.emplace(EPipeline::EnvFilter, VK_NULL_HANDLE);

  colorBlendInfo.attachmentCount = 1;
  colorBlendInfo.pAttachments = &colorBlendAttachment;

  graphicsPipelineInfo.layout = getPipelineLayout(EPipelineLayout::Environment);
  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::Environment);

  getRenderPass(ERenderPass::Environment)->viewport = viewport;
  getRenderPass(ERenderPass::Environment)->scissor = scissor;

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::EnvFilter)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create environment filter graphics pipeline.");

    return RE_CRITICAL;
  }

  for (auto stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  // 'EnvIrradiance' pipeline
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

  // 'EnvLUT' pipeline
  shaderStages.clear();
  shaderStages = {loadShader("vs_brdfLUT.spv", VK_SHADER_STAGE_VERTEX_BIT),
                  loadShader("fs_brdfLUT.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  system.pipelines.emplace(EPipeline::LUTGen, VK_NULL_HANDLE);

  vertexInputInfo = VkPipelineVertexInputStateCreateInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  graphicsPipelineInfo.layout = getPipelineLayout(EPipelineLayout::LUTGen);
  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::LUTGen);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::LUTGen)) != VK_SUCCESS) {
    RE_LOG(Critical,
           "Failed to create LUT generator pipeline.");

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

VkPipelineLayout& core::MRenderer::getPipelineLayout(EPipelineLayout type) {
  return system.layouts.at(type);
}

VkPipeline& core::MRenderer::getPipeline(EPipeline type) {
  // not error checked
  return system.pipelines.at(type);
}

bool core::MRenderer::checkPipeline(uint32_t pipelineFlags,
                                    EPipeline pipelineFlag) {
  return pipelineFlags & pipelineFlag;
}

TResult core::MRenderer::configureRenderPasses() {
  // NOTE: order of pipeline 'emplacement' is important

  // configure deferred render pass that generates G buffer targets
  RRenderPass* pRenderPass = getRenderPass(ERenderPass::Deferred);
  pRenderPass->usedLayout = getPipelineLayout(EPipelineLayout::Scene);
  pRenderPass->usedPipelines.emplace_back(EPipeline::OpaqueCullBack);
  pRenderPass->usedPipelines.emplace_back(EPipeline::OpaqueCullNone);
  pRenderPass->usedPipelines.emplace_back(EPipeline::BlendCullNone);  // TODO: check if there any issues with blending with skybox

  // configure main 'scene', PBR render pass
  pRenderPass = getRenderPass(ERenderPass::PBR);
  pRenderPass->usedLayout = getPipelineLayout(EPipelineLayout::Scene);
  pRenderPass->usedPipelines.emplace_back(EPipeline::PBRDeferred);
  //pRenderPass->usedPipelines.emplace_back(EPipeline::Skybox);

  pRenderPass = getRenderPass(ERenderPass::Present);
  pRenderPass->usedLayout = getPipelineLayout(EPipelineLayout::Scene);
  pRenderPass->usedPipelines.emplace_back(EPipeline::Present);

  // configure 'environment' cubemap render pass
  pRenderPass = getRenderPass(ERenderPass::Environment);
  pRenderPass->usedLayout = getPipelineLayout(EPipelineLayout::Environment);
  pRenderPass->usedPipelines.emplace_back(EPipeline::EnvFilter);
  pRenderPass->usedPipelines.emplace_back(EPipeline::EnvIrradiance);
  pRenderPass->viewport.width =
      static_cast<float>(core::vulkan::envFilterExtent);
  pRenderPass->viewport.height = -pRenderPass->viewport.width;
  pRenderPass->viewport.y = pRenderPass->viewport.width;

  return RE_OK;
}