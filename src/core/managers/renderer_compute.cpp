#include "pch.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/time.h"
#include "core/managers/renderer.h"

void core::MRenderer::updateComputeImageSet(std::vector<RTexture*>* pInImages, std::vector<RTexture*>* pInSamplers,
  const bool useExtraImageViews, const bool useExtraSamplerViews) {
  if (!pInImages) {
    RE_LOG(Error, "Failed to update compute image descriptor set. No data was provided.");
    return;
  }

  const uint32_t imageWriteSize = static_cast<uint32_t>(pInImages->size());
  uint32_t samplerWriteSize = 0;
  uint32_t extraImageWriteSize = 0;
  uint32_t extraSamplerWriteSize = 0;

  if (useExtraImageViews) {
    for (auto& image : *pInImages) {
      extraImageWriteSize += static_cast<uint32_t>(image->texture.extraViews.size());
    }
  }
  
  if (pInSamplers) {
    samplerWriteSize = static_cast<uint32_t>(pInSamplers->size());

    if (useExtraSamplerViews) {
      for (auto& sampler : *pInSamplers) {
        extraImageWriteSize += static_cast<uint32_t>(sampler->texture.extraViews.size());
      }
    }
  }

  const uint32_t writeCount = imageWriteSize + samplerWriteSize + extraImageWriteSize + extraSamplerWriteSize;

  if (writeCount > config::scene::storageImageBudget || writeCount == 0) {
    return;
  }

  VkWriteDescriptorSet defaultWriteSet{};
  defaultWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  defaultWriteSet.dstSet = compute.imageDescriptorSet;
  defaultWriteSet.dstBinding = 0;
  defaultWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  defaultWriteSet.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets(writeCount, VkWriteDescriptorSet{});
  uint32_t arrayIndex = 0u;

  for (uint32_t i = 0; i < imageWriteSize; ++i) {
    writeDescriptorSets[arrayIndex] = defaultWriteSet;
    writeDescriptorSets[arrayIndex].dstArrayElement = arrayIndex;
    writeDescriptorSets[arrayIndex].pImageInfo = &(*pInImages)[i]->texture.imageInfo;
    
    arrayIndex++;

    if (useExtraImageViews && !(*pInImages)[i]->texture.extraViews.empty()) {
      RTexture* pImage = (*pInImages)[i];
      const uint32_t extraViewCount = static_cast<uint32_t>(pImage->texture.extraViews.size());

      for (uint32_t viewIndex = 0; viewIndex < extraViewCount; ++viewIndex) {
        pImage->texture.extraViews[viewIndex].imageLayout = pImage->texture.imageLayout;

        writeDescriptorSets[arrayIndex] = defaultWriteSet;
        writeDescriptorSets[arrayIndex].dstArrayElement = arrayIndex;
        writeDescriptorSets[arrayIndex].pImageInfo = &pImage->texture.extraViews[viewIndex];

        arrayIndex++;
      }
    }
  }

  if (pInSamplers) {
    defaultWriteSet.dstBinding = 1;
    defaultWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    for (uint32_t j = 0; j < samplerWriteSize; ++j) {
      writeDescriptorSets[arrayIndex] = defaultWriteSet;
      writeDescriptorSets[arrayIndex].dstArrayElement = j;
      writeDescriptorSets[arrayIndex].pImageInfo = &(*pInSamplers)[j]->texture.imageInfo;

      arrayIndex++;

      if (useExtraSamplerViews && !(*pInSamplers)[j]->texture.extraViews.empty()) {
        RTexture* pSampler = (*pInSamplers)[j];
        const uint32_t extraViewCount = static_cast<uint32_t>(pSampler->texture.extraViews.size());

        for (uint32_t viewIndex = 0; viewIndex < extraViewCount; ++viewIndex) {
          pSampler->texture.extraViews[viewIndex].imageLayout = pSampler->texture.imageLayout;

          writeDescriptorSets[arrayIndex] = defaultWriteSet;
          writeDescriptorSets[arrayIndex].dstArrayElement = arrayIndex;
          writeDescriptorSets[arrayIndex].pImageInfo = &pSampler->texture.extraViews[viewIndex];

          arrayIndex++;
        }
      }
    }
  }

  vkUpdateDescriptorSets(core::renderer.logicalDevice.device, writeCount,
    writeDescriptorSets.data(), 0, nullptr);

  // TODO: Update this accounting for samplers also being bound now
  compute.imagePCB.imageIndex = 0;
  compute.imagePCB.imageCount = writeCount;
}

void core::MRenderer::updateComputeBufferSet(std::vector<RBuffer*>* pInBuffers) {
  if (!pInBuffers || pInBuffers->empty()) {
    RE_LOG(Error, "Failed to update compute buffer descriptor set. No data was provided.");
    return;
  }

  const uint32_t writeCount = static_cast<uint32_t>(pInBuffers->size());

  VkWriteDescriptorSet defaultWriteSet{};
  defaultWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  defaultWriteSet.dstSet = compute.bufferDescriptorSet;
  defaultWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  defaultWriteSet.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeSets(writeCount, defaultWriteSet);
  std::vector<VkDescriptorBufferInfo> bufferDescriptors(writeCount);

  for (uint8_t currentWrite = 0; currentWrite < writeCount; ++currentWrite) {
    bufferDescriptors[currentWrite].buffer = (*pInBuffers)[currentWrite]->buffer;
    bufferDescriptors[currentWrite].offset = 0u;
    bufferDescriptors[currentWrite].range = VK_WHOLE_SIZE;

    writeSets[currentWrite].pBufferInfo = &bufferDescriptors[currentWrite];
    writeSets[currentWrite].dstBinding = currentWrite;
  }

  vkUpdateDescriptorSets(logicalDevice.device, writeCount, writeSets.data(), 0, nullptr);
}

void core::MRenderer::executeComputeImage(VkCommandBuffer commandBuffer,
  EComputePipeline pipeline) {
  VkPipelineLayout layout = getPipelineLayout(EPipelineLayout::ComputeImage);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    getComputePipeline(pipeline));
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    layout, 0, 1, &compute.imageDescriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
    sizeof(RComputeImagePCB), &compute.imagePCB);

  vkCmdDispatch(commandBuffer, compute.imageExtent.width, compute.imageExtent.height, compute.imageExtent.depth);
}

void core::MRenderer::generateBRDFMap() {
  RE_LOG(Log, "Generating BRDF LUT map to '%s' texture.", RTGT_BRDFMAP);

  core::time.tickTimer();

  RTexture* pLUTTexture = core::resources.getTexture(RTGT_BRDFMAP);
  std::vector<RTexture*> pTextures;
  pTextures.emplace_back(pLUTTexture);

  updateComputeImageSet(&pTextures, 0);

  VkCommandBuffer commandBuffer = createCommandBuffer(
    ECmdType::Compute, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  vkCmdPushConstants(commandBuffer, getPipelineLayout(EPipelineLayout::ComputeImage),
    VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(RComputeImagePCB), &compute.imagePCB);

  executeComputeImage(commandBuffer, EComputePipeline::ImageLUT);

  VkImageSubresourceRange subRange{};
  subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subRange.baseArrayLayer = 0;
  subRange.layerCount = 1;
  subRange.baseMipLevel = 0;
  subRange.levelCount = 1;

  setImageLayout(commandBuffer, pLUTTexture,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subRange);

  flushCommandBuffer(commandBuffer, ECmdType::Compute, true, false);

  // update renderer descriptor sets to use texture as LUT source
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstSet = core::renderer.getSceneDescriptorSet(i);
    writeDescriptorSet.dstBinding = 4;
    writeDescriptorSet.pImageInfo =
      &core::resources.getTexture(RTGT_BRDFMAP)->texture.imageInfo;

    vkUpdateDescriptorSets(core::renderer.logicalDevice.device, 1,
      &writeDescriptorSet, 0, nullptr);
  }

  float timeSpent = core::time.tickTimer();
  RE_LOG(Log, "Generating BRDF LUT map took %.4f milliseconds.", timeSpent);
}

void core::MRenderer::queueComputeJob(RComputeJobInfo* pInfo) {
  if (pInfo) {
    if (pInfo->depth == 0) pInfo->depth = 1;

    compute.jobs.emplace_back(*pInfo);
  }
}

void core::MRenderer::executeComputeJobImmediate(RComputeJobInfo* pInfo) {
  VkCommandBuffer computeBuffer = command.buffersCompute[renderView.frameInFlight];

  switch (pInfo->jobType) {
    case EComputeJob::Image: {
      VkImageSubresourceRange range{};
      range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      range.baseArrayLayer = 0;
      range.baseMipLevel = 0;

      compute.imageExtent.width = pInfo->width;
      compute.imageExtent.height = pInfo->height;
      compute.imageExtent.depth = pInfo->depth;
      compute.imagePCB.intValues = pInfo->intValues;
      compute.imagePCB.floatValues = pInfo->floatValues;

      for (auto& image : pInfo->pImageAttachments) {
        range.layerCount = image->texture.layerCount;
        range.levelCount = image->texture.levelCount;

        setImageLayout(computeBuffer, image, VK_IMAGE_LAYOUT_GENERAL, range);
      }

      for (auto& sampler : pInfo->pSamplerAttachments) {
        range.layerCount = sampler->texture.layerCount;
        range.levelCount = sampler->texture.levelCount;

        setImageLayout(computeBuffer, sampler, VK_IMAGE_LAYOUT_GENERAL, range);
      }

      updateComputeImageSet(&pInfo->pImageAttachments, &pInfo->pSamplerAttachments, pInfo->useExtraImageViews, pInfo->useExtraSamplerViews);

      executeComputeImage(computeBuffer, pInfo->pipeline);

      VkMemoryBarrier2 memoryBarrier{};
      memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
      memoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
      memoryBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
      memoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
      memoryBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

      VkDependencyInfo computeDependency{};
      computeDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
      computeDependency.memoryBarrierCount = 1u;
      computeDependency.pMemoryBarriers = &memoryBarrier;

      vkCmdPipelineBarrier2(computeBuffer, &computeDependency);

      if (pInfo->transtionToShaderReadOnly) {
        for (auto& image : pInfo->pImageAttachments) {
          range.layerCount = image->texture.layerCount;
          range.levelCount = image->texture.levelCount;

          setImageLayout(computeBuffer, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
        }

        for (auto& sampler : pInfo->pSamplerAttachments) {
          range.layerCount = sampler->texture.layerCount;
          range.levelCount = sampler->texture.levelCount;

          setImageLayout(computeBuffer, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
        }
      }

      return;
    }

    case EComputeJob::Buffer: {
      VkImageSubresourceRange range{};
      range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      range.baseArrayLayer = 0;
      range.baseMipLevel = 0;

      compute.imageExtent.width = pInfo->width;
      compute.imageExtent.height = pInfo->height;
      compute.imageExtent.depth = pInfo->depth;
      compute.imagePCB.intValues = pInfo->intValues;
      compute.imagePCB.floatValues = pInfo->floatValues;

      for (auto& image : pInfo->pImageAttachments) {
        range.layerCount = image->texture.layerCount;
        range.levelCount = image->texture.levelCount;

        setImageLayout(computeBuffer, image, VK_IMAGE_LAYOUT_GENERAL, range);
      }

      for (auto& sampler : pInfo->pSamplerAttachments) {
        range.layerCount = sampler->texture.layerCount;
        range.levelCount = sampler->texture.levelCount;

        setImageLayout(computeBuffer, sampler, VK_IMAGE_LAYOUT_GENERAL, range);
      }

      updateComputeImageSet(&pInfo->pImageAttachments, &pInfo->pSamplerAttachments, pInfo->useExtraImageViews, pInfo->useExtraSamplerViews);
      updateComputeBufferSet(&pInfo->pBufferAttachments);

      return;
    }
  }
}

void core::MRenderer::executeQueuedComputeJobs() {
  if (compute.jobs.empty()) return;

  beginCommandBuffer(command.buffersCompute[renderView.frameInFlight], false);

  RComputeJobInfo* pInfo = &compute.jobs.front();
  executeComputeJobImmediate(pInfo);
  compute.jobs.erase(compute.jobs.begin());

  flushCommandBuffer(command.buffersCompute[renderView.frameInFlight], ECmdType::Compute);
}