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
  return createFramebuffer(ERenderPass::Environment, {RTGT_ENVSRC}, RFB_ENV);
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

  return RE_OK;
}

void core::MRenderer::destroyRenderPasses() {
  RE_LOG(Log, "Destroying render passes.");

  for (auto& it : system.renderPasses) {
    vkDestroyRenderPass(logicalDevice.device, it.second.renderPass, nullptr);
  }
}

TResult core::MRenderer::configureRenderPasses() {
  system.renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  system.renderPassBeginInfo.renderArea.offset = {0, 0};
  system.renderPassBeginInfo.renderArea.extent = swapchain.imageExtent;

  // must be set up during frame drawing
  system.renderPassBeginInfo.renderPass = VK_NULL_HANDLE;
  system.renderPassBeginInfo.framebuffer = VK_NULL_HANDLE;

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
  getRenderPass(ERenderPass::Deferred)->scissor = scissor;
  getRenderPass(ERenderPass::Present)->viewport = viewport;
  getRenderPass(ERenderPass::Present)->scissor = scissor;

  viewport.y = static_cast<float>(config::shadowResolution);
  viewport.width = viewport.y;
  viewport.height = -viewport.y;
  scissor.extent = {config::shadowResolution, config::shadowResolution};

  getRenderPass(ERenderPass::Shadow)->viewport = viewport;
  getRenderPass(ERenderPass::Shadow)->scissor = scissor;

  viewport.y = core::vulkan::envFilterExtent;
  viewport.width = viewport.y;
  viewport.height = -viewport.y;
  scissor.extent = {core::vulkan::envFilterExtent,
                    core::vulkan::envFilterExtent};

  getRenderPass(ERenderPass::Environment)->viewport = viewport;
  getRenderPass(ERenderPass::Environment)->scissor = scissor;

  return RE_OK;
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

TResult core::MRenderer::createPipelineLayouts() {
  RE_LOG(Log, "Creating graphics pipelines.");

  //
  // PIPELINE LAYOUTS
  //

  // common 3D pipeline data //
#ifndef NDEBUG
  RE_LOG(Log, "Setting up common 3D pipeline data.");
#endif

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
      getDescriptorSetLayout(EDescriptorSetLayout::Model),    // 1
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

#ifndef NDEBUG
  RE_LOG(Log, "Creating pipeline layout for \"Compute Image\".");
#endif

  layoutType = EPipelineLayout::ComputeImage;
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

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));
  }

  // Environment / image based lighting pipelines
  {
    // 'EnvFilter' pipeline
    RGraphicsPipelineInfo pipelineInfo{};
    pipelineInfo.pipeline = EPipeline::EnvFilter;
    pipelineInfo.pipelineLayout = EPipelineLayout::Environment;
    pipelineInfo.renderPass = ERenderPass::Environment;
    pipelineInfo.vertexShader = "vs_environment.spv";
    pipelineInfo.fragmentShader = "fs_envFilter.spv";
    pipelineInfo.colorBlendAttachmentCount = 1;

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));

    // 'EnvIrradiance' pipeline
    pipelineInfo.pipeline = EPipeline::EnvIrradiance;
    pipelineInfo.fragmentShader = "fs_envIrradiance.spv";

    RE_CHECK(createGraphicsPipeline(&pipelineInfo));
  }

  return RE_OK;
}

void core::MRenderer::destroyGraphicsPipelines() {
  RE_LOG(Log, "Shutting down graphics pipeline.");
  destroyDescriptorSetLayouts();

  for (auto& pipeline : system.graphicsPipelines) {
    vkDestroyPipeline(logicalDevice.device, pipeline.second, nullptr);
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

TResult core::MRenderer::createGraphicsPipeline(
    RGraphicsPipelineInfo* pipelineInfo) {
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
  viewportInfo.pViewports = &getRenderPass(pipelineInfo->renderPass)->viewport;
  viewportInfo.viewportCount = 1;
  viewportInfo.pScissors = &getRenderPass(pipelineInfo->renderPass)->scissor;
  viewportInfo.scissorCount = 1;

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
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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
  graphicsPipelineInfo.renderPass = getVkRenderPass(pipelineInfo->renderPass);
  graphicsPipelineInfo.subpass = pipelineInfo->subpass;
  graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  graphicsPipelineInfo.basePipelineIndex = -1;

  system.graphicsPipelines.emplace(pipelineInfo->pipeline, VK_NULL_HANDLE);

  if (vkCreateGraphicsPipelines(
          logicalDevice.device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo,
          nullptr,
          &getGraphicsPipeline(pipelineInfo->pipeline)) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create pipeline E%d.", pipelineInfo->pipeline);

    return RE_CRITICAL;
  }

  for (auto stage : shaderStages) {
    vkDestroyShaderModule(logicalDevice.device, stage.module, nullptr);
  }

  RE_LOG(Log, "Created pipeline E%d.", pipelineInfo->pipeline);
  return RE_OK;
}

TResult core::MRenderer::createComputePipeline(
    RComputePipelineInfo* pipelineInfo) {
  return RE_OK;
}

VkPipeline& core::MRenderer::getGraphicsPipeline(EPipeline type) {
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

TResult core::MRenderer::configureRenderer() {
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