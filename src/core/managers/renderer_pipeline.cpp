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

  return RE_OK;
}

TResult core::MRenderer::createPipelineLayouts() {
  RE_LOG(Log, "Creating graphics pipelines.");

  //
  // PIPELINE LAYOUTS
  //

  // common 3D pipeline data //
#ifndef NDEBUG
  RE_LOG(Log, "Setting up common 3D pipeline data.");
#endif

  VkPushConstantRange sceneVertexPCR{};
  sceneVertexPCR.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  sceneVertexPCR.offset = 0;
  sceneVertexPCR.size = sizeof(RSceneVertexPCB);

  VkPushConstantRange sceneFragmentPCR{};
  sceneFragmentPCR.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  sceneFragmentPCR.offset = sizeof(RSceneVertexPCB);
  sceneFragmentPCR.size = sizeof(RSceneFragmentPCB);

  VkPushConstantRange scenePCRanges[] = { sceneVertexPCR, sceneFragmentPCR };

#ifndef NDEBUG
  RE_LOG(Log, "Creating pipeline layout for \"scene\".");
#endif

  EPipelineLayout layoutType = EPipelineLayout::Scene;
  system.layouts.emplace(layoutType, VK_NULL_HANDLE);

  // pipeline layout for the main 'scene'
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
      getDescriptorSetLayout(EDescriptorSetLayout::Scene),      // 0
      getDescriptorSetLayout(EDescriptorSetLayout::Model),      // 1
      getDescriptorSetLayout(EDescriptorSetLayout::MaterialEXT) // 2
  };

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  layoutInfo.pSetLayouts = descriptorSetLayouts.data();
  layoutInfo.pushConstantRangeCount = 2;
  layoutInfo.pPushConstantRanges = scenePCRanges;

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
  layoutInfo.pPushConstantRanges = VK_NULL_HANDLE;

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
      getDescriptorSetLayout(EDescriptorSetLayout::Scene),  // 0
      getDescriptorSetLayout(EDescriptorSetLayout::Model),        // 1
      getDescriptorSetLayout(EDescriptorSetLayout::MaterialEXT)   // 2
  };

  VkPushConstantRange envVertexPCR{};
  envVertexPCR.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  envVertexPCR.offset = 0;
  envVertexPCR.size = sizeof(RSceneVertexPCB);

  VkPushConstantRange envFragmentPCR{};
  envFragmentPCR.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  envFragmentPCR.offset = sizeof(RSceneVertexPCB);
  envFragmentPCR.size = sizeof(REnvironmentFragmentPCB);

  VkPushConstantRange envPCRanges[] = { envVertexPCR, envFragmentPCR };

  layoutInfo = VkPipelineLayoutCreateInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  layoutInfo.pSetLayouts = descriptorSetLayouts.data();
  layoutInfo.pushConstantRangeCount = 2;
  layoutInfo.pPushConstantRanges = envPCRanges;

  if (vkCreatePipelineLayout(logicalDevice.device, &layoutInfo, nullptr,
                             &getPipelineLayout(layoutType)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create \"environment\" pipeline layout.");

    return RE_CRITICAL;
  }

#ifndef NDEBUG
  RE_LOG(Log, "Creating pipeline layout for \"Compute Image\".");
#endif

  layoutType = EPipelineLayout::ComputeImage;
  system.layouts.emplace(layoutType, VK_NULL_HANDLE);

  descriptorSetLayouts.clear();
  descriptorSetLayouts = {
      getDescriptorSetLayout(EDescriptorSetLayout::ComputeImage)
  };

  VkPushConstantRange computeImagePushConstRange{};
  computeImagePushConstRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  computeImagePushConstRange.offset = 0;
  computeImagePushConstRange.size = sizeof(RComputeImagePCB);

  layoutInfo = VkPipelineLayoutCreateInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  layoutInfo.pSetLayouts = descriptorSetLayouts.data();
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &computeImagePushConstRange;

  if (vkCreatePipelineLayout(logicalDevice.device, &layoutInfo, nullptr,
                             &getPipelineLayout(layoutType)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create \"Compute Image\" pipeline layout.");

    return RE_CRITICAL;
  }

  return RE_OK;
}

TResult core::MRenderer::createGraphicsPipelines() {
  // G-Buffer pipelines
  {
    // backface culled opaque pipeline
    RGraphicsPipelineInfo pipelineInfo{};
    pipelineInfo.pipeline = EPipeline::OpaqueCullBack;
    pipelineInfo.pipelineLayout = EPipelineLayout::Scene;
    pipelineInfo.renderPass = ERenderPass::Deferred;
    pipelineInfo.subpass = 0;
    pipelineInfo.vertexShader = "vs_scene.spv";
    pipelineInfo.fragmentShader = "fs_gbuffer.spv";
    pipelineInfo.colorBlendAttachmentCount =
        static_cast<uint32_t>(scene.pGBufferTargets.size());
    pipelineInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineInfo.blendEnable = VK_FALSE;
    pipelineInfo.viewportId = EViewport::vpMain;

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));

    // non-culled opaque pipeline
    pipelineInfo.pipeline = EPipeline::OpaqueCullNone;
    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.blendEnable = VK_FALSE;

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));

    // non-culled blending pipeline
    pipelineInfo.pipeline = EPipeline::BlendCullNone;
    pipelineInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineInfo.blendEnable = VK_TRUE;

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));

    // TODO: add mask pipeline
  }
    
  // PBR pipeline, uses the result of G-Buffer pipelines
  {
    RGraphicsPipelineInfo pipelineInfo{};
    pipelineInfo.pipeline = EPipeline::PBR;
    pipelineInfo.pipelineLayout = EPipelineLayout::PBR;
    pipelineInfo.renderPass = ERenderPass::Deferred;
    pipelineInfo.subpass = 1;
    pipelineInfo.vertexShader = "vs_quad.spv";
    pipelineInfo.fragmentShader = "fs_pbr.spv";
    pipelineInfo.colorBlendAttachmentCount = 1;
    pipelineInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineInfo.blendEnable = VK_FALSE;
    pipelineInfo.viewportId = EViewport::vpMain;

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));
  }

  // Skybox pipeline, uses forward subpass of the deferred render pass
  {
    RGraphicsPipelineInfo pipelineInfo{};
    pipelineInfo.pipeline = EPipeline::Skybox;
    pipelineInfo.pipelineLayout = EPipelineLayout::Scene;
    pipelineInfo.renderPass = ERenderPass::Deferred;
    pipelineInfo.subpass = 2;
    pipelineInfo.vertexShader = "vs_skybox.spv";
    pipelineInfo.fragmentShader = "fs_skybox.spv";
    pipelineInfo.colorBlendAttachmentCount = 1;
    pipelineInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineInfo.blendEnable = VK_FALSE;
    pipelineInfo.viewportId = EViewport::vpMain;

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));
  }

  // "Shadow" depth map pipeline
  {
    RGraphicsPipelineInfo pipelineInfo{};
    pipelineInfo.pipeline = EPipeline::Shadow;
    pipelineInfo.pipelineLayout = EPipelineLayout::Scene;
    pipelineInfo.renderPass = ERenderPass::Shadow;
    pipelineInfo.vertexShader = "vs_scene.spv";
    pipelineInfo.fragmentShader = "fs_shadowPass.spv";
    pipelineInfo.colorBlendAttachmentCount = 0;
    pipelineInfo.blendEnable = VK_TRUE;
    pipelineInfo.viewportId = EViewport::vpShadow;

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));
  }

  // "Present" final output pipeline
  {
    RGraphicsPipelineInfo pipelineInfo{};
    pipelineInfo.pipeline = EPipeline::Present;
    pipelineInfo.pipelineLayout = EPipelineLayout::Scene;
    pipelineInfo.renderPass = ERenderPass::Present;
    pipelineInfo.vertexShader = "vs_quad.spv";
    pipelineInfo.fragmentShader = "fs_present.spv";
    pipelineInfo.colorBlendAttachmentCount = 1;
    pipelineInfo.viewportId = EViewport::vpMain;

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));
  }

  // Environment / image based lighting pipelines
  {
    // 'EnvFilter' pipeline
    RGraphicsPipelineInfo pipelineInfo{};
    pipelineInfo.pipeline = EPipeline::EnvFilter;
    //pipelineInfo.pipelineLayout = EPipelineLayout::Environment;
    pipelineInfo.pipelineLayout = EPipelineLayout::Scene;
    pipelineInfo.renderPass = ERenderPass::Null;
    pipelineInfo.dynamicRenderPass = EDynamicRenderingPass::Environment;
    pipelineInfo.vertexShader = "vs_environment.spv";
    pipelineInfo.fragmentShader = "fs_envFilter.spv";
    pipelineInfo.colorBlendAttachmentCount = 1u;
    pipelineInfo.viewportId = EViewport::vpEnvFilter;

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));

    // 'EnvIrradiance' pipeline
    pipelineInfo.pipeline = EPipeline::EnvIrradiance;
    pipelineInfo.fragmentShader = "fs_envIrradiance.spv";
    pipelineInfo.colorBlendAttachmentCount = 1u;
    pipelineInfo.viewportId = EViewport::vpEnvIrrad;

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));
  }

  return RE_OK;
}

void core::MRenderer::destroyGraphicsPipelines() {
  RE_LOG(Log, "Shutting down graphics pipeline.");
  destroyDescriptorSetLayouts();

  for (auto& pipeline : system.graphicsPipelines) {
    vkDestroyPipeline(logicalDevice.device, pipeline.second.pipeline, nullptr);
  }

  for (auto& layout : system.layouts) {
    vkDestroyPipelineLayout(logicalDevice.device, layout.second, nullptr);
  }
}

TResult core::MRenderer::createComputePipelines() {
  RE_LOG(Log, "Creating compute pipelines.");

  //
  // Compute Image pipeline for creating BRDF LUT image
  //
  {
    system.computePipelines.emplace(EComputePipeline::ImageLUT, VK_NULL_HANDLE);

    VkPipelineShaderStageCreateInfo shaderStage =
        loadShader("cs_brdfLUT.spv", VK_SHADER_STAGE_COMPUTE_BIT);

    // 'Compute Image' pipeline
    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &core::vulkan::formatLUT;
    pipelineRenderingInfo.viewMask = 0;

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.layout =
        getPipelineLayout(EPipelineLayout::ComputeImage);
    computePipelineInfo.stage = shaderStage;
    computePipelineInfo.flags = 0;

    if (vkCreateComputePipelines(
            logicalDevice.device, VK_NULL_HANDLE, 1, &computePipelineInfo,
            nullptr,
            &getComputePipeline(EComputePipeline::ImageLUT)) != VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create Compute Image 'BRDF LUT' pipeline.");

      return RE_CRITICAL;
    }

    vkDestroyShaderModule(logicalDevice.device, shaderStage.module, nullptr);
  }

  //
  // Compute Image pipeline for mipmapping R16G16B16A16_SFLOAT image
  //
  {
    system.computePipelines.emplace(EComputePipeline::ImageMipMap16f,
                                    VK_NULL_HANDLE);

    VkPipelineShaderStageCreateInfo shaderStage =
        loadShader("cs_mipmap16f.spv", VK_SHADER_STAGE_COMPUTE_BIT);

    // 'Compute Image' pipeline
    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &core::vulkan::formatHDR16;
    pipelineRenderingInfo.viewMask = 0;

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.layout =
        getPipelineLayout(EPipelineLayout::ComputeImage);
    computePipelineInfo.stage = shaderStage;
    computePipelineInfo.flags = 0;

    if (vkCreateComputePipelines(
            logicalDevice.device, VK_NULL_HANDLE, 1, &computePipelineInfo,
            nullptr, &getComputePipeline(EComputePipeline::ImageMipMap16f)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create Compute Image 'MipMap16f' pipeline.");

      return RE_CRITICAL;
    }

    vkDestroyShaderModule(logicalDevice.device, shaderStage.module, nullptr);
  }

  return RE_OK;
}

void core::MRenderer::destroyComputePipelines() {
  for (auto& pipeline : system.computePipelines) {
    vkDestroyPipeline(logicalDevice.device, pipeline.second, nullptr);
  }
}

VkPipelineLayout& core::MRenderer::getPipelineLayout(EPipelineLayout type) {
  return system.layouts.at(type);
}

TResult core::MRenderer::createGraphicsPipeline(RGraphicsPipelineInfo* pipelineInfo) {
  // Create pipeline for dynamic rendering and ignore usual render pass settings if dynamic rendering pass variable was defined
  const bool isDynamicRendering = pipelineInfo->dynamicRenderPass != EDynamicRenderingPass::Null;

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  inputAssemblyInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyInfo.topology = pipelineInfo->primitiveTopology;
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampleInfo{};
  multisampleInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleInfo.sampleShadingEnable = VK_FALSE;
  multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampleInfo.minSampleShading = 1.0f;
  multisampleInfo.pSampleMask = nullptr;
  multisampleInfo.alphaToCoverageEnable = VK_FALSE;
  multisampleInfo.alphaToOneEnable = VK_FALSE;

  VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
  rasterizationInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationInfo.depthClampEnable = VK_FALSE;
  rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterizationInfo.polygonMode = pipelineInfo->polygonMode;
  rasterizationInfo.lineWidth = 1.0f;
  rasterizationInfo.cullMode = pipelineInfo->cullMode;
  rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizationInfo.depthBiasEnable = VK_FALSE;
  rasterizationInfo.depthBiasConstantFactor = 0.0f;
  rasterizationInfo.depthBiasClamp = 0.0f;
  rasterizationInfo.depthBiasSlopeFactor = 0.0f;

  // dynamic states allow changing specific parts of the pipeline without
  // recreating it
  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.dynamicStateCount =
      static_cast<uint32_t>(dynamicStates.size());
  dynamicStateInfo.pDynamicStates = dynamicStates.data();

  VkPipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportInfo.viewportCount = 1;
  viewportInfo.scissorCount = 1;
  viewportInfo.pViewports = &system.viewports[pipelineInfo->viewportId].viewport;
  viewportInfo.pScissors = &system.viewports[pipelineInfo->viewportId].scissor;

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
  colorBlendAttachment.blendEnable = pipelineInfo->blendEnable;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
  colorBlendAttachments.resize(pipelineInfo->colorBlendAttachmentCount);

  for (uint8_t i = 0; i < pipelineInfo->colorBlendAttachmentCount; ++i) {
    colorBlendAttachments[i] = colorBlendAttachment;
  }

  VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
  colorBlendInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendInfo.logicOpEnable = VK_FALSE;
  colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
  colorBlendInfo.attachmentCount =
      static_cast<uint32_t>(colorBlendAttachments.size());
  colorBlendInfo.pAttachments = colorBlendAttachments.data();
  colorBlendInfo.blendConstants[0] = 0.0f;
  colorBlendInfo.blendConstants[1] = 0.0f;
  colorBlendInfo.blendConstants[2] = 0.0f;
  colorBlendInfo.blendConstants[3] = 0.0f;

  VkVertexInputBindingDescription vertexBindingDesc = RVertex::getBindingDesc();
  const auto& attributeDescs = RVertex::getAttributeDescs();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  if (!pipelineInfo->vertexShader.empty()) {
    shaderStages.emplace_back(
        loadShader(pipelineInfo->vertexShader.c_str(), VK_SHADER_STAGE_VERTEX_BIT));
  }
  if (!pipelineInfo->fragmentShader.empty()) {
    shaderStages.emplace_back(loadShader(pipelineInfo->fragmentShader.c_str(),
                                         VK_SHADER_STAGE_FRAGMENT_BIT));
  }

  if (shaderStages.empty()) {
    RE_LOG(Critical, "Failed to create pipeline E%d. No shaders were loaded.",
           pipelineInfo->pipeline);
    return RE_CRITICAL;
  }

  VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
  graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
  graphicsPipelineInfo.pMultisampleState = &multisampleInfo;
  graphicsPipelineInfo.pRasterizationState = &rasterizationInfo;
  graphicsPipelineInfo.pDynamicState = &dynamicStateInfo;
  graphicsPipelineInfo.pViewportState = &viewportInfo;
  graphicsPipelineInfo.pColorBlendState = &colorBlendInfo;
  graphicsPipelineInfo.pDepthStencilState = &depthStencilInfo;
  graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
  graphicsPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  graphicsPipelineInfo.pStages = shaderStages.data();
  graphicsPipelineInfo.layout = getPipelineLayout(pipelineInfo->pipelineLayout);
  graphicsPipelineInfo.renderPass = (isDynamicRendering) ? nullptr : getVkRenderPass(pipelineInfo->renderPass);
  graphicsPipelineInfo.subpass = (isDynamicRendering) ? 0u : pipelineInfo->subpass;
  graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  graphicsPipelineInfo.basePipelineIndex = -1;

  if (isDynamicRendering) {
    RPipeline* pPipeline = getDynamicRenderingPass(pipelineInfo->dynamicRenderPass)->getPipeline(pipelineInfo->pipeline);

    if (pPipeline) {
      graphicsPipelineInfo.pNext = &pPipeline->dynamic.pipelineCreateInfo;
      getDynamicRenderingPass(pipelineInfo->dynamicRenderPass)->layout = graphicsPipelineInfo.layout;
    } else {
      RE_LOG(Error, "Failed to configure dynamic rendering pass %d, pipeline %d. Seems like provided pipeline does not belong to this pass.",
        pipelineInfo->dynamicRenderPass, pipelineInfo->pipeline);
      return RE_CRITICAL;
    }
  }

  // add new pipeline structure and fill it with data
  system.graphicsPipelines.emplace(pipelineInfo->pipeline, RPipeline{});

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr,
          &getGraphicsPipeline(pipelineInfo->pipeline).pipeline) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create pipeline E%d.", pipelineInfo->pipeline);

    return RE_CRITICAL;
  }

  system.graphicsPipelines.at(pipelineInfo->pipeline).pipelineId = pipelineInfo->pipeline;
  system.graphicsPipelines.at(pipelineInfo->pipeline).subpassIndex = pipelineInfo->subpass;

  for (auto& stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  RE_LOG(Log, "Created pipeline E%d.", pipelineInfo->pipeline);
  return RE_OK;
}

TResult core::MRenderer::createComputePipeline(RComputePipelineInfo* pipelineInfo) {
  return RE_OK;
}

RPipeline& core::MRenderer::getGraphicsPipeline(EPipeline type) {
  // not error checked
  return system.graphicsPipelines.at(type);
}

VkPipeline& core::MRenderer::getComputePipeline(EComputePipeline type) {
  // not error checked
  return system.computePipelines.at(type);
}

bool core::MRenderer::checkPipeline(uint32_t pipelineFlags,
                                    EPipeline pipelineFlag) {
  return pipelineFlags & pipelineFlag;
}