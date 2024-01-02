#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/renderer.h"

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
                            &swapchain.framebuffers[i].framebuffer) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "failed to create swapchain framebuffer with index %d.",
             i);

      return RE_CRITICAL;
    }
  }

  TResult chkResult;
  std::vector<std::string> attachmentNames;
  attachmentNames.emplace_back(RTGT_SHADOW);
  if (chkResult = createFramebuffer(ERenderPass::Shadow, attachmentNames,
                                    RFB_SHADOW) != RE_OK) {
    return chkResult;
  }

  // Deferred subpass 0
  attachmentNames.clear();
  attachmentNames.emplace_back(RTGT_GPOSITION); // 0
  attachmentNames.emplace_back(RTGT_GDIFFUSE);  // 1
  attachmentNames.emplace_back(RTGT_GNORMAL);   // 2
  attachmentNames.emplace_back(RTGT_GPHYSICAL); // 3
  attachmentNames.emplace_back(RTGT_GEMISSIVE); // 4
  attachmentNames.emplace_back(RTGT_GPBR);      // 5 - subpasses 1 and 2
  attachmentNames.emplace_back(RTGT_DEPTH);     // depth buffer target

  if (chkResult = createFramebuffer(ERenderPass::Deferred, attachmentNames,
                                    RFB_DEFERRED) != RE_OK) {
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
  colorAttachment.format = swapchain.formatData.format;
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
  // then they are combined into 1 PBR color attachment

  ERenderPass passType = ERenderPass::Deferred;
  system.renderPasses.emplace(passType, RRenderPass{});
  RE_LOG(Log, "Creating render pass E%d", passType);

  colorAttachment.format = core::vulkan::formatHDR16;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // deferred render targets: position, color, normal, physical, emissive
  // + PBR render target
  uint32_t attachmentCount = 6;
  std::vector<VkAttachmentDescription> deferredColorAttachments(attachmentCount, colorAttachment);

  VkRenderPass newRenderPass = createRenderPass(
      logicalDevice.device, attachmentCount, deferredColorAttachments.data(),
      &depthAttachment, passType);
  system.renderPasses.at(passType).renderPass = newRenderPass;
  system.renderPasses.at(passType).clearValues =
      fGetClearValues(clearColor, attachmentCount, clearDepth);

  //
  // PRESENT render pass
  // the 16 bit float color map is rendered to the compatible surface target
  passType = ERenderPass::Present;
  system.renderPasses.emplace(passType, RRenderPass{});
  RE_LOG(Log, "Creating render pass E%d", passType);

  colorAttachment.format = swapchain.formatData.format;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  newRenderPass = createRenderPass(logicalDevice.device, 1, &colorAttachment,
                                   &depthAttachment, passType);
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

  newRenderPass = createRenderPass(logicalDevice.device, 1, &colorAttachment,
                                   nullptr, passType);
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
  newRenderPass = createRenderPass(logicalDevice.device, 1, &colorAttachment,
                                   nullptr, passType);
  system.renderPasses.at(passType).renderPass = newRenderPass;
  system.renderPasses.at(passType).clearValues =
      fGetClearValues(clearColor, 1, clearDepth);

  //
  // SHADOW render pass
  //
  passType = ERenderPass::Shadow;
  system.renderPasses.emplace(passType, RRenderPass{});
  RE_LOG(Log, "Creating render pass E%d", passType);

  depthAttachment.format = core::vulkan::formatShadow;

  newRenderPass = createRenderPass(logicalDevice.device, 0, nullptr,
                                   &depthAttachment, passType);
  system.renderPasses.at(passType).renderPass = newRenderPass;
  system.renderPasses.at(passType).clearValues =
      fGetClearValues(clearColor, 0, clearDepth);

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

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchain.imageExtent;

  getRenderPass(ERenderPass::Deferred)->viewport = viewport;
  getRenderPass(ERenderPass::Present)->viewport = viewport;

  getRenderPass(ERenderPass::Deferred)->scissor = scissor;
  getRenderPass(ERenderPass::Present)->scissor = scissor;

  viewport.y = static_cast<float>(config::shadowResolution);
  viewport.width = viewport.y;
  viewport.height = -viewport.y;
  scissor.extent = {config::shadowResolution, config::shadowResolution};

  getRenderPass(ERenderPass::Shadow)->viewport = viewport;
  getRenderPass(ERenderPass::Shadow)->scissor = scissor;

  VkPipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportInfo.pViewports = &getRenderPass(ERenderPass::Deferred)->viewport;
  viewportInfo.viewportCount = 1;
  viewportInfo.pScissors = &getRenderPass(ERenderPass::Deferred)->scissor;
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
  RE_LOG(Log, "Creating pipeline layout for \"PBR\".");
#endif

  layoutType = EPipelineLayout::PBR;
  system.layouts.emplace(layoutType, VK_NULL_HANDLE);

  descriptorSetLayouts.clear();
  descriptorSetLayouts = {
      getDescriptorSetLayout(EDescriptorSetLayout::Scene),    // 0
      getDescriptorSetLayout(EDescriptorSetLayout::Model),    // 1
      getDescriptorSetLayout(EDescriptorSetLayout::PBRInput)  // 2
  };

  layoutInfo = VkPipelineLayoutCreateInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  layoutInfo.pSetLayouts = descriptorSetLayouts.data();
  layoutInfo.pushConstantRangeCount = 0;

  if (vkCreatePipelineLayout(logicalDevice.device, &layoutInfo, nullptr,
                             &getPipelineLayout(layoutType)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create \"PBR\" pipeline layout.");

    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Creating pipeline layout for \"Environment\".");
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
    RE_LOG(Critical, "Failed to create \"environment\" pipeline layout.");

    return RE_CRITICAL;
  }

  // pipeline layout for generating BRDF LUT map
#ifndef NDEBUG
  RE_LOG(Log, "Creating pipeline layout for \"BRDF LUT\".");
#endif

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
    RE_LOG(Critical, "Failed to create \"BRDF LUT generator\" pipeline layout.");

    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Creating pipeline layout for \"Compute Image\".");
#endif

  layoutType = EPipelineLayout::Compute;
  system.layouts.emplace(layoutType, VK_NULL_HANDLE);

  descriptorSetLayouts.clear();
  descriptorSetLayouts = {
      getDescriptorSetLayout(EDescriptorSetLayout::ComputeImage)};

  layoutInfo = VkPipelineLayoutCreateInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  layoutInfo.pSetLayouts = descriptorSetLayouts.data();
  layoutInfo.pushConstantRangeCount = 0;
  layoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(logicalDevice.device, &layoutInfo, nullptr,
                             &getPipelineLayout(layoutType)) != VK_SUCCESS) {
    RE_LOG(Critical,
           "Failed to create \"Compute Image\" pipeline layout.");

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
      loadShader("vs_scene.spv", VK_SHADER_STAGE_VERTEX_BIT),
      loadShader("fs_gbuffer.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

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
  shaderStages = {loadShader("vs_quad.spv", VK_SHADER_STAGE_VERTEX_BIT),
                  loadShader("fs_pbr.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::Deferred);
  graphicsPipelineInfo.subpass = 1;
  graphicsPipelineInfo.layout = getPipelineLayout(EPipelineLayout::PBR);
  rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendInfo.attachmentCount = 1;
  colorBlendInfo.pAttachments = &colorBlendAttachment;

  system.pipelines.emplace(EPipeline::PBR, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::PBR)) != VK_SUCCESS) {
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

  graphicsPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  graphicsPipelineInfo.pStages = shaderStages.data();
  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::Deferred);
  graphicsPipelineInfo.subpass = 2;
  graphicsPipelineInfo.layout = getPipelineLayout(EPipelineLayout::Scene);
  rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendInfo.attachmentCount = 1;
  colorBlendInfo.pAttachments = &colorBlendAttachment;

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

  // 'Shadow' depth map pipeline
  shaderStages.clear();
  shaderStages = {
      loadShader("vs_scene.spv", VK_SHADER_STAGE_VERTEX_BIT),
      loadShader("fs_shadowPass.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::Shadow);
  graphicsPipelineInfo.subpass = 0;
  graphicsPipelineInfo.layout = getPipelineLayout(EPipelineLayout::Scene);
  rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendInfo.attachmentCount = 0;

  system.pipelines.emplace(EPipeline::Shadow, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr, &getPipeline(EPipeline::Shadow)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create shadow depth map pipeline.");

    return RE_CRITICAL;
  }

  for (auto& stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  // 'Present' final output pipeline
  shaderStages.clear();
  shaderStages = {loadShader("vs_quad.spv", VK_SHADER_STAGE_VERTEX_BIT),
                  loadShader("fs_present.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

  system.pipelines.emplace(EPipeline::Present, VK_NULL_HANDLE);

  graphicsPipelineInfo.renderPass = getVkRenderPass(ERenderPass::Present);
  rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendInfo.attachmentCount = 1;

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

TResult core::MRenderer::createComputePipelines() {
  RE_LOG(Log, "Creating compute pipelines.");

  //
  // Compute Image pipeline
  //
  system.pipelines.emplace(EPipeline::Compute, VK_NULL_HANDLE);

  VkPipelineShaderStageCreateInfo shaderStage =
      loadShader("cs_test.spv", VK_SHADER_STAGE_COMPUTE_BIT);

  // 'Compute Image' pipeline
  VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
  pipelineRenderingInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  pipelineRenderingInfo.colorAttachmentCount = 1;
  pipelineRenderingInfo.pColorAttachmentFormats = &core::vulkan::formatHDR32;
  pipelineRenderingInfo.viewMask = 0;

  VkComputePipelineCreateInfo computePipelineInfo{};
  computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineInfo.layout = getPipelineLayout(EPipelineLayout::Compute);
  computePipelineInfo.stage = shaderStage;
  computePipelineInfo.flags = 0;

  if (vkCreateComputePipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &computePipelineInfo,
          nullptr, &getPipeline(EPipeline::Compute)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create Compute Image pipeline.");

    return RE_CRITICAL;
  }

  vkDestroyShaderModule(logicalDevice.device, shaderStage.module, nullptr);

  return RE_OK;
}

void core::MRenderer::destroyComputePipelines() {
  vkDestroyPipeline(logicalDevice.device, getPipeline(EPipeline::Compute),
                    nullptr);
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

  /* SHADOW */
  RRenderPass* pRenderPass = getRenderPass(ERenderPass::Shadow);
  pRenderPass->usedLayout = getPipelineLayout(EPipelineLayout::Scene);
  pRenderPass->usedPipelines.emplace_back(EPipeline::Shadow);

  /* DEFERRED */
  pRenderPass = getRenderPass(ERenderPass::Deferred);
  pRenderPass->usedLayout = getPipelineLayout(EPipelineLayout::Scene);
  pRenderPass->usedPipelines.emplace_back(EPipeline::OpaqueCullBack, 0);
  pRenderPass->usedPipelines.emplace_back(EPipeline::OpaqueCullNone, 0);
  pRenderPass->usedPipelines.emplace_back(EPipeline::BlendCullNone, 0);
  pRenderPass->usedPipelines.emplace_back(EPipeline::Skybox, 2);

  /* PRESENT */
  // uses swapchain framebuffers
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