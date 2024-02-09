#include "pch.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/renderer.h"

TResult core::MRenderer::createDescriptorPool() {
  RE_LOG(Log, "Creating descriptor pool.");

  // the number of descriptors in the given pool per set/layout type
  std::vector<VkDescriptorPoolSize> poolSizes;
  poolSizes.resize(3);

  // model view projection matrix
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  // model nodes
  // TODO: rewrite so that descriptorCounts are calculated without hardcoding
  poolSizes[0].descriptorCount += 2000 * MAX_FRAMES_IN_FLIGHT;

  // materials and textures
  // TODO: rewrite so that descriptorCounts are calculated by objects/textures
  // using map data e.g. max textures that are going to be loaded
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = 2000 * MAX_FRAMES_IN_FLIGHT;

  // compute image descriptors
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[2].descriptorCount = 10;

  uint32_t maxSets = 0;
  for (uint8_t i = 0; i < poolSizes.size(); ++i) {
    maxSets += poolSizes[i].descriptorCount;
  }
  maxSets += 100;  // descriptor set headroom

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = maxSets;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;

  if (vkCreateDescriptorPool(logicalDevice.device, &poolInfo, nullptr,
                             &system.descriptorPool) != VK_SUCCESS) {
    RE_LOG(Critical, "Failed to create descriptor pool.");
    return RE_CRITICAL;
  }

  return RE_OK;
}

void core::MRenderer::destroyDescriptorPool() {
  RE_LOG(Log, "Destroying descriptor pool.");
  vkDestroyDescriptorPool(logicalDevice.device, system.descriptorPool, nullptr);
}

const VkDescriptorPool core::MRenderer::getDescriptorPool() {
  return system.descriptorPool;
}

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

  // MaterialEXT descriptor layout
  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::MaterialEXT,
      VK_NULL_HANDLE);

    const VkDescriptorBindingFlagsEXT bindingFlags =
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsCreateInfo{};
    bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    bindingFlagsCreateInfo.bindingCount = 1;
    bindingFlagsCreateInfo.pBindingFlags = &bindingFlags;

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        config::scene::sampledImageBudget,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount =
      static_cast<uint32_t>(setLayoutBindings.size());
    setLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    setLayoutCreateInfo.pNext = &bindingFlagsCreateInfo;

    if (vkCreateDescriptorSetLayout(
      logicalDevice.device, &setLayoutCreateInfo, nullptr,
      &system.descriptorSetLayouts.at(EDescriptorSetLayout::MaterialEXT)) !=
      VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create extended material descriptor set layout.");
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

  // Compute image processing layout

  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::ComputeImage,
                                        VK_NULL_HANDLE);

    const VkDescriptorBindingFlagsEXT bindingFlagsImage =
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

    const VkDescriptorBindingFlagsEXT bindingFlagsSampler =
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

    const VkDescriptorBindingFlagsEXT bindingFlags[] = {bindingFlagsImage, bindingFlagsSampler};

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsCreateInfo{};
    bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    bindingFlagsCreateInfo.bindingCount = 1;
    bindingFlagsCreateInfo.pBindingFlags = bindingFlags;


    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      config::scene::storageImageBudget,
      VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
      {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      config::scene::storageImageBudget,
      VK_SHADER_STAGE_COMPUTE_BIT, nullptr}
    };

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

const VkDescriptorSetLayout core::MRenderer::getDescriptorSetLayout(
    EDescriptorSetLayout type) const {
  return system.descriptorSetLayouts.at(type);
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

    scene.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateDescriptorSets(logicalDevice.device, &setAllocInfo,
                                 scene.descriptorSets.data()) != VK_SUCCESS) {
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
      writeDescriptorSets[0].dstSet = scene.descriptorSets[i];
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
      writeDescriptorSets[1].dstSet = scene.descriptorSets[i];
      writeDescriptorSets[1].dstBinding = 1;
      writeDescriptorSets[1].pBufferInfo = &descriptorBufferInfoLighting;

      // environment image data

      // environment maps are created with a layout for accepting data writes
      // however descriptor sets require info about their final state
      VkDescriptorImageInfo imageDescriptors[3]{
          core::resources.getTexture(RTGT_ENVFILTER)->texture.imageInfo,
          core::resources.getTexture(RTGT_ENVIRRAD)->texture.imageInfo,
          core::resources.getTexture(RTGT_BRDFMAP)->texture.imageInfo};

      for (VkDescriptorImageInfo& imageInfo : imageDescriptors) {
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }

      // RTGT_ENVFILTER
      writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[2].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[2].descriptorCount = 1;
      writeDescriptorSets[2].dstSet = scene.descriptorSets[i];
      writeDescriptorSets[2].dstBinding = 2;
      writeDescriptorSets[2].pImageInfo = &imageDescriptors[0];

      // RTGT_ENVIRRAD
      writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[3].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[3].descriptorCount = 1;
      writeDescriptorSets[3].dstSet = scene.descriptorSets[i];
      writeDescriptorSets[3].dstBinding = 3;
      writeDescriptorSets[3].pImageInfo = &imageDescriptors[1];

      // RTGT_BRDFMAP
      writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[4].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[4].descriptorCount = 1;
      writeDescriptorSets[4].dstSet = scene.descriptorSets[i];
      writeDescriptorSets[4].dstBinding = 4;
      writeDescriptorSets[4].pImageInfo = &imageDescriptors[2];

      vkUpdateDescriptorSets(logicalDevice.device, descriptorCount,
                             writeDescriptorSets.data(), 0, nullptr);
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
  RE_LOG(Log, "Creating extended material descriptor set.");
#endif
  {
    VkDescriptorSetLayout MaterialEXTLayout =
      core::renderer.getDescriptorSetLayout(EDescriptorSetLayout::MaterialEXT);

    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableAllocateInfo{};
    variableAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
    variableAllocateInfo.descriptorSetCount = 1;
    variableAllocateInfo.pDescriptorCounts = &core::vulkan::maxSampler2DDescriptors;

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = core::renderer.getDescriptorPool();
    allocateInfo.pSetLayouts = &MaterialEXTLayout;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pNext = &variableAllocateInfo;

    if (vkAllocateDescriptorSets(core::renderer.logicalDevice.device,
      &allocateInfo, &material.descriptorSet) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to allocate descriptor set for extended material storage.");
      return RE_CRITICAL;
    };
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
                                 &compute.imageDescriptorSet) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to allocate descriptor set for PBR input subpass.");
      return RE_CRITICAL;
    };
  }

  return RE_OK;
}