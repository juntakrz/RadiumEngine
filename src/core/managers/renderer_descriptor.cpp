#include "pch.h"
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
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
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

  // standard RMaterial set of 6 texture maps
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

  // PBR input layout
  // 0 - color input
  // 1 - normal input
  // 2 - physical input (metalness, roughness, ambient occlusion)
  // 3 - position input
  // 4 - emissive input
  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::PBRInput,
                                        VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
        static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.at(EDescriptorSetLayout::PBRInput)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create PBR input descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  // model transform matrices
  // 0 - model transform matrices (offset stored in entity)
  // 1 - per node transform matrices (offset SHOULD BE stored by animations
  // manager) 2 - per model inverse bind matrices (offset SHOULD BE stored by
  // animations manager)
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

  // Environment map generation
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

  // Compute image processing layout

  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::ComputeImage,
                                        VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
         VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
        static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.at(EDescriptorSetLayout::ComputeImage)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create compute image descriptor set layout.");
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
      writeDescriptorSets[0].descriptorType =
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
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
    if ((result = vkAllocateDescriptorSets(device, &descriptorSetAllocInfo,
                                           &scene.transformDescriptorSet)) !=
        VK_SUCCESS) {
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
    rootMatrixBufferInfo.range = sizeof(glm::mat4);  // root matrix

    VkDescriptorBufferInfo nodeMatrixBufferInfo{};
    nodeMatrixBufferInfo.buffer = scene.nodeTransformBuffer.buffer;
    nodeMatrixBufferInfo.offset = 0;
    nodeMatrixBufferInfo.range = config::scene::nodeBlockSize;

    VkDescriptorBufferInfo skinningMatricesBufferInfo{};
    skinningMatricesBufferInfo.buffer = scene.skinTransformBuffer.buffer;
    skinningMatricesBufferInfo.offset = 0;
    skinningMatricesBufferInfo.range = config::scene::skinBlockSize;

    std::vector<VkWriteDescriptorSet> writeSets(3);
    writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writeSets[0].descriptorCount = 1;
    writeSets[0].dstSet = scene.transformDescriptorSet;
    writeSets[0].dstBinding = 0;
    writeSets[0].pBufferInfo = &rootMatrixBufferInfo;

    writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writeSets[1].descriptorCount = 1;
    writeSets[1].dstSet = scene.transformDescriptorSet;
    writeSets[1].dstBinding = 1;
    writeSets[1].pBufferInfo = &nodeMatrixBufferInfo;

    writeSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writeSets[2].descriptorCount = 1;
    writeSets[2].dstSet = scene.transformDescriptorSet;
    writeSets[2].dstBinding = 2;
    writeSets[2].pBufferInfo = &skinningMatricesBufferInfo;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()),
                           writeSets.data(), 0, nullptr);
  }

#ifndef NDEBUG
  RE_LOG(Log, "Creating deferred PBR descriptor set.");
#endif
  {
    // Allocate PBR input's descriptor set
    VkDescriptorSetLayout PBRInputLayout =
        core::renderer.getDescriptorSetLayout(EDescriptorSetLayout::PBRInput);

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = core::renderer.getDescriptorPool();
    allocateInfo.pSetLayouts = &PBRInputLayout;
    allocateInfo.descriptorSetCount = 1;

    if (vkAllocateDescriptorSets(core::renderer.logicalDevice.device,
                                 &allocateInfo,
                                 &scene.GBufferDescriptorSet) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to allocate descriptor set for PBR input subpass.");
      return RE_CRITICAL;
    };

    std::vector<VkDescriptorImageInfo> imageDescriptors;

    for (const auto& image : scene.pGBufferTargets) {
      imageDescriptors.emplace_back(image->texture.descriptor);
    }

    // Write retrieved data to newly allocated descriptor set
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    uint32_t writeSize = static_cast<uint32_t>(imageDescriptors.size());
    writeDescriptorSets.resize(writeSize);

    for (uint32_t i = 0; i < writeSize; ++i) {
      // If an image is a color attachment - it's probably a render target that
      // will be used as a shader data source at some point
      if (imageDescriptors[i].imageLayout ==
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        imageDescriptors[i].imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }

      writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[i].dstSet = scene.GBufferDescriptorSet;
      writeDescriptorSets[i].dstBinding = i;
      writeDescriptorSets[i].descriptorType =
          VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
      writeDescriptorSets[i].descriptorCount = 1;
      writeDescriptorSets[i].pImageInfo = &imageDescriptors[i];
    }

    vkUpdateDescriptorSets(core::renderer.logicalDevice.device, writeSize,
                           writeDescriptorSets.data(), 0, nullptr);
  }

#ifndef NDEBUG
  RE_LOG(Log, "Creating compute image descriptor set.");
#endif
  {
    // allocate material's descriptor set
    VkDescriptorSetLayout computeImageLayout =
        core::renderer.getDescriptorSetLayout(EDescriptorSetLayout::ComputeImage);

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = core::renderer.getDescriptorPool();
    allocateInfo.pSetLayouts = &computeImageLayout;
    allocateInfo.descriptorSetCount = 1;

    if (vkAllocateDescriptorSets(core::renderer.logicalDevice.device,
                                 &allocateInfo,
                                 &compute.computeImageDescriptorSet) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to allocate descriptor set for PBR input subpass.");
      return RE_CRITICAL;
    };

    std::vector<VkDescriptorImageInfo> imageDescriptors;
    imageDescriptors.emplace_back(compute.pComputeImageTarget->texture.descriptor);

    // write retrieved data to newly allocated descriptor set
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    uint32_t writeSize = static_cast<uint32_t>(imageDescriptors.size());
    writeDescriptorSets.resize(writeSize);

    for (uint32_t i = 0; i < writeSize; ++i) {
      writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[i].dstSet = compute.computeImageDescriptorSet;
      writeDescriptorSets[i].dstBinding = i;
      writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      writeDescriptorSets[i].descriptorCount = 1;
      writeDescriptorSets[i].pImageInfo = &imageDescriptors[i];
    }

    vkUpdateDescriptorSets(core::renderer.logicalDevice.device, writeSize,
                           writeDescriptorSets.data(), 0, nullptr);
  }
  return RE_OK;
}