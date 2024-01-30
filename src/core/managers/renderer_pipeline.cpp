#include "pch.h"
#include "vk_mem_alloc.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/renderer.h"

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

  layoutInfo = VkPipelineLayoutCreateInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  layoutInfo.pSetLayouts = descriptorSetLayouts.data();
  layoutInfo.pushConstantRangeCount = 2;
  layoutInfo.pPushConstantRanges = scenePCRanges;

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

VkPipelineLayout& core::MRenderer::getPipelineLayout(EPipelineLayout type) {
  return system.layouts.at(type);
}

TResult core::MRenderer::createComputePipelines() {
  RE_LOG(Log, "Creating compute pipelines.");

  VkComputePipelineCreateInfo computePipelineInfo{};
  computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineInfo.layout =
    getPipelineLayout(EPipelineLayout::ComputeImage);
  computePipelineInfo.flags = 0;

  //
  // Compute Image pipeline for creating BRDF LUT image
  //
  {
    VkPipelineShaderStageCreateInfo shaderStage =
        loadShader("cs_brdfLUT.spv", VK_SHADER_STAGE_COMPUTE_BIT);
    computePipelineInfo.stage = shaderStage;

    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &core::vulkan::formatLUT;
    pipelineRenderingInfo.viewMask = 0;

    system.computePipelines.emplace(EComputePipeline::ImageLUT, VK_NULL_HANDLE);

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
    VkPipelineShaderStageCreateInfo shaderStage =
        loadShader("cs_mipmap16f.spv", VK_SHADER_STAGE_COMPUTE_BIT);
    computePipelineInfo.stage = shaderStage;

    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &core::vulkan::formatHDR16;
    pipelineRenderingInfo.viewMask = 0;

    system.computePipelines.emplace(EComputePipeline::ImageMipMap16f,
      VK_NULL_HANDLE);

    if (vkCreateComputePipelines(
            logicalDevice.device, VK_NULL_HANDLE, 1, &computePipelineInfo,
            nullptr, &getComputePipeline(EComputePipeline::ImageMipMap16f)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create Compute Image 'MipMap16f' pipeline.");

      return RE_CRITICAL;
    }

    vkDestroyShaderModule(logicalDevice.device, shaderStage.module, nullptr);
  }

  //
  // Compute Image pipeline for processing environmental irradiance cubemap
  //
  {
    VkPipelineShaderStageCreateInfo shaderStage =
      loadShader("cs_envIrrad.spv", VK_SHADER_STAGE_COMPUTE_BIT);
    computePipelineInfo.stage = shaderStage;

    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &core::vulkan::formatHDR16;
    pipelineRenderingInfo.viewMask = 0;

    system.computePipelines.emplace(EComputePipeline::ImageEnvIrradiance,
      VK_NULL_HANDLE);

    if (vkCreateComputePipelines(
      logicalDevice.device, VK_NULL_HANDLE, 1, &computePipelineInfo,
      nullptr, &getComputePipeline(EComputePipeline::ImageEnvIrradiance)) !=
      VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create Compute Image 'Environment Irradiance' pipeline.");

      return RE_CRITICAL;
    }

    vkDestroyShaderModule(logicalDevice.device, shaderStage.module, nullptr);
  }

  //
  // Compute Image pipeline for processing environmental prefiltered cubemap
  //
  {
    VkPipelineShaderStageCreateInfo shaderStage =
      loadShader("cs_envFilter.spv", VK_SHADER_STAGE_COMPUTE_BIT);
    computePipelineInfo.stage = shaderStage;

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.layout =
      getPipelineLayout(EPipelineLayout::ComputeImage);
    computePipelineInfo.stage = shaderStage;
    computePipelineInfo.flags = 0;

    system.computePipelines.emplace(EComputePipeline::ImageEnvFilter,
      VK_NULL_HANDLE);

    if (vkCreateComputePipelines(
      logicalDevice.device, VK_NULL_HANDLE, 1, &computePipelineInfo,
      nullptr, &getComputePipeline(EComputePipeline::ImageEnvFilter)) !=
      VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create Compute Image 'Environment Prefiltered' pipeline.");

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

TResult core::MRenderer::createGraphicsPipeline(RGraphicsPipelineInfo* pipelineInfo) {
  RDynamicRenderingPass* pRenderPass = pipelineInfo->pRenderPass;
  const uint32_t colorAttachmentCount = pipelineInfo->pDynamicPipelineInfo->colorAttachmentCount;

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
  rasterizationInfo.depthBiasEnable = pipelineInfo->depthBias.enable;
  rasterizationInfo.depthBiasConstantFactor = pipelineInfo->depthBias.constantFactor;
  rasterizationInfo.depthBiasSlopeFactor = pipelineInfo->depthBias.slopeFactor;
  rasterizationInfo.depthBiasClamp = pipelineInfo->depthBias.clamp;

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
  viewportInfo.pViewports = &system.viewports[pRenderPass->viewportId].viewport;
  viewportInfo.pScissors = &system.viewports[pRenderPass->viewportId].scissor;

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
  colorBlendAttachment.blendEnable = pipelineInfo->enableBlending;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
  colorBlendAttachments.resize(colorAttachmentCount);

  for (uint8_t i = 0; i < colorAttachmentCount; ++i) {
    colorBlendAttachments[i] = colorBlendAttachment;
  }

  VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
  colorBlendInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendInfo.logicOpEnable = VK_FALSE;
  colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
  colorBlendInfo.attachmentCount = colorAttachmentCount;
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
    RE_LOG(Critical, "Failed to create pipeline for dynamic render pass E%d. No shaders were loaded.",
           pRenderPass->passId);
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
  graphicsPipelineInfo.layout = pRenderPass->layout;
  graphicsPipelineInfo.renderPass = nullptr;
  graphicsPipelineInfo.subpass = 0u;
  graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  graphicsPipelineInfo.basePipelineIndex = -1;
  graphicsPipelineInfo.pNext = pipelineInfo->pDynamicPipelineInfo;

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr,
          &pRenderPass->pipeline) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create pipeline for dynamic rendering pass E%d.", pRenderPass->passId);

    return RE_CRITICAL;
  }

  for (auto& stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  return RE_OK;
}

VkPipeline& core::MRenderer::getComputePipeline(EComputePipeline type) {
  // not error checked
  return system.computePipelines.at(type);
}

bool core::MRenderer::checkPass(uint32_t passFlags, EDynamicRenderingPass passFlag) {
  return passFlags & (uint32_t)passFlag;
}