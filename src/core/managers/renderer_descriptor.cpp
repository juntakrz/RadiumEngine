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

  // Scene matrices and environmental maps
  // 0 - MVP matrix
  // 1 - Lighting data
  // 2 - Environment filtered map
  // 3 - Environment irradiance map
  // 4 - Generated BRDF LUT map
  // 5 - Ambient occlusion noise map
  // 6 - General data storage
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
      {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };
    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.at(EDescriptorSetLayout::Scene)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create scene descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  // MaterialEXT descriptor layout
  // 0 - Material push block array
  // 1 - Combined image samplers

  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::MaterialEXT, VK_NULL_HANDLE);

    const VkDescriptorBindingFlagsEXT bindingFlags =
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        config::scene::sampledImageBudget,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };

    const VkDescriptorBindingFlagsEXT bindingFlagsArray[2] = { 0, bindingFlags };

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsCreateInfo{};
    bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    bindingFlagsCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());;
    //bindingFlagsCreateInfo.pBindingFlags = &bindingFlags;
    bindingFlagsCreateInfo.pBindingFlags = bindingFlagsArray;

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
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

  // Model transform matrices
  // 0 - Model transform matrices (offsets are stored by AEntity)
  // 1 - Per node transform matrices
  // 2 - Per model inverse bind matrices
  // 3 - Transparency linked list data buffer
  // 4 - Transparency linked list image
  // 5 - Transparency linked list node buffer
  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::Model,
                                        VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1,
        VK_SHADER_STAGE_VERTEX_BIT, nullptr},
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1,
        VK_SHADER_STAGE_VERTEX_BIT, nullptr},
      {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1,
        VK_SHADER_STAGE_VERTEX_BIT, nullptr},
      {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

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
    setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
            logicalDevice.device, &setLayoutCreateInfo, nullptr,
            &system.descriptorSetLayouts.at(EDescriptorSetLayout::ComputeImage)) !=
        VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create compute image descriptor set layout.");
      return RE_CRITICAL;
    }
  }

  // Compute buffer processing layout

  {
    system.descriptorSetLayouts.emplace(EDescriptorSetLayout::ComputeBuffer,
      VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
      VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
      VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
      {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
      VK_SHADER_STAGE_COMPUTE_BIT, nullptr}
    };

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.pBindings = setLayoutBindings.data();
    setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(
      logicalDevice.device, &setLayoutCreateInfo, nullptr,
      &system.descriptorSetLayouts.at(EDescriptorSetLayout::ComputeBuffer)) !=
      VK_SUCCESS) {
      RE_LOG(Critical, "Failed to create compute buffer descriptor set layout.");
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

    uint32_t descriptorCount = 7u;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      // model*view*projection data for descriptor set
      VkDescriptorBufferInfo descriptorBufferInfoMVP;
      descriptorBufferInfoMVP.buffer = scene.sceneBuffers[i].buffer;
      descriptorBufferInfoMVP.offset = 0;
      descriptorBufferInfoMVP.range = sizeof(RSceneUBO);

      // lighting data for descriptor set
      VkDescriptorBufferInfo descriptorBufferInfoLighting;
      descriptorBufferInfoLighting.buffer = lighting.buffers[i].buffer;
      descriptorBufferInfoLighting.offset = 0;
      descriptorBufferInfoLighting.range = sizeof(RLightingUBO);

      std::vector<VkWriteDescriptorSet> writeDescriptorSets(descriptorCount);

      // Settings used for writing to MVP descriptor set
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

      // Settings used for writing to lighting descriptor set
      writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeDescriptorSets[1].descriptorCount = 1;
      writeDescriptorSets[1].dstSet = scene.descriptorSets[i];
      writeDescriptorSets[1].dstBinding = 1;
      writeDescriptorSets[1].pBufferInfo = &descriptorBufferInfoLighting;

      // Environment image data

      // Environment maps are created with a layout for accepting data writes
      // however descriptor sets require info about their final state
      VkDescriptorImageInfo imageDescriptors[4]{
          core::resources.getTexture(RTGT_ENVFILTER)->texture.imageInfo,
          core::resources.getTexture(RTGT_ENVIRRAD)->texture.imageInfo,
          core::resources.getTexture(RTGT_BRDFMAP)->texture.imageInfo,
          core::resources.getTexture(RTGT_NOISEMAP)->texture.imageInfo
      };

      for (VkDescriptorImageInfo& imageInfo : imageDescriptors) {
        //imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
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

      // RTGT_NOISEMAP
      writeDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[5].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[5].descriptorCount = 1;
      writeDescriptorSets[5].dstSet = scene.descriptorSets[i];
      writeDescriptorSets[5].dstBinding = 5;
      writeDescriptorSets[5].pImageInfo = &imageDescriptors[3];

      // General storage buffer for various static / not often changed data
      VkDescriptorBufferInfo generalBufferInfo{};
      generalBufferInfo.buffer = scene.generalBuffer.buffer;
      generalBufferInfo.offset = 0;
      generalBufferInfo.range = VK_WHOLE_SIZE;

      writeDescriptorSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      writeDescriptorSets[6].descriptorCount = 1;
      writeDescriptorSets[6].dstSet = scene.descriptorSets[i];
      writeDescriptorSets[6].dstBinding = 6;
      writeDescriptorSets[6].pBufferInfo = &generalBufferInfo;

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

    // 0
    VkDescriptorBufferInfo rootMatrixBufferInfo{};
    rootMatrixBufferInfo.buffer = scene.rootTransformBuffer.buffer;
    rootMatrixBufferInfo.offset = 0;
    rootMatrixBufferInfo.range = VK_WHOLE_SIZE;  // root matrix

    // 1
    VkDescriptorBufferInfo nodeMatrixBufferInfo{};
    nodeMatrixBufferInfo.buffer = scene.nodeTransformBuffer.buffer;
    nodeMatrixBufferInfo.offset = 0;
    nodeMatrixBufferInfo.range = VK_WHOLE_SIZE;

    // 2
    VkDescriptorBufferInfo skinningMatricesBufferInfo{};
    skinningMatricesBufferInfo.buffer = scene.skinTransformBuffer.buffer;
    skinningMatricesBufferInfo.offset = 0;
    skinningMatricesBufferInfo.range = VK_WHOLE_SIZE;

    // 3
    VkDescriptorBufferInfo transparencyLLBufferDataInfo{};
    transparencyLLBufferDataInfo.buffer = scene.transparencyLinkedListDataBuffer.buffer;
    transparencyLLBufferDataInfo.offset = 0;
    transparencyLLBufferDataInfo.range = VK_WHOLE_SIZE;

    // 4
    VkDescriptorImageInfo transparencyLLImageInfo{};
    transparencyLLImageInfo.imageView = scene.pTransparencyStorageTexture->texture.view;
    transparencyLLImageInfo.imageLayout = scene.pTransparencyStorageTexture->texture.imageLayout;
    transparencyLLImageInfo.sampler = scene.pTransparencyStorageTexture->texture.sampler;

    // 5
    VkDescriptorBufferInfo transparencyLLBufferInfo{};
    transparencyLLBufferInfo.buffer = scene.transparencyLinkedListBuffer.buffer;
    transparencyLLBufferInfo.offset = 0;
    transparencyLLBufferInfo.range = VK_WHOLE_SIZE;

    std::vector<VkWriteDescriptorSet> writeSets(6);
    writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    writeSets[0].descriptorCount = 1;
    writeSets[0].dstSet = scene.transformDescriptorSet;
    writeSets[0].dstBinding = 0;
    writeSets[0].pBufferInfo = &rootMatrixBufferInfo;

    writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    writeSets[1].descriptorCount = 1;
    writeSets[1].dstSet = scene.transformDescriptorSet;
    writeSets[1].dstBinding = 1;
    writeSets[1].pBufferInfo = &nodeMatrixBufferInfo;

    writeSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    writeSets[2].descriptorCount = 1;
    writeSets[2].dstSet = scene.transformDescriptorSet;
    writeSets[2].dstBinding = 2;
    writeSets[2].pBufferInfo = &skinningMatricesBufferInfo;

    writeSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeSets[3].descriptorCount = 1;
    writeSets[3].dstSet = scene.transformDescriptorSet;
    writeSets[3].dstBinding = 3;
    writeSets[3].pBufferInfo = &transparencyLLBufferDataInfo;

    writeSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeSets[4].descriptorCount = 1;
    writeSets[4].dstSet = scene.transformDescriptorSet;
    writeSets[4].dstBinding = 4;
    writeSets[4].pImageInfo = &transparencyLLImageInfo;

    writeSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeSets[5].descriptorCount = 1;
    writeSets[5].dstSet = scene.transformDescriptorSet;
    writeSets[5].dstBinding = 5;
    writeSets[5].pBufferInfo = &transparencyLLBufferInfo;

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

#ifndef NDEBUG
    RE_LOG(Log, "Adding material block data buffer to material descriptor set.");
#endif

    VkDevice device = core::renderer.logicalDevice.device;

    VkDescriptorBufferInfo materialDataBufferInfo{};
    materialDataBufferInfo.buffer = material.buffer.buffer;
    materialDataBufferInfo.offset = 0;
    materialDataBufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet writeSet{};
    writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeSet.descriptorCount = 1;
    writeSet.dstSet = material.descriptorSet;
    writeSet.dstBinding = 0;
    writeSet.pBufferInfo = &materialDataBufferInfo;

    vkUpdateDescriptorSets(device, 1u, &writeSet, 0, nullptr);
  }

#ifndef NDEBUG
  RE_LOG(Log, "Creating compute image descriptor set.");
#endif
  {
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

#ifndef NDEBUG
  RE_LOG(Log, "Creating compute buffer descriptor set.");
#endif
  {
    VkDescriptorSetLayout computeBufferLayout =
      core::renderer.getDescriptorSetLayout(EDescriptorSetLayout::ComputeBuffer);

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = core::renderer.getDescriptorPool();
    allocateInfo.pSetLayouts = &computeBufferLayout;
    allocateInfo.descriptorSetCount = 1;

    if (vkAllocateDescriptorSets(core::renderer.logicalDevice.device,
      &allocateInfo,
      &compute.bufferDescriptorSet) != VK_SUCCESS) {
      RE_LOG(Error, "Failed to allocate descriptor set for PBR input subpass.");
      return RE_CRITICAL;
    };
  }

  return RE_OK;
}