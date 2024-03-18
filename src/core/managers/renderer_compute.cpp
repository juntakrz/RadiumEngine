#include "pch.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/time.h"
#include "core/managers/renderer.h"

void core::MRenderer::updateComputeDescriptorSets(RComputeJobInfo* pJobInfo) {
  if (!pJobInfo) {
    RE_LOG(Error, "Failed to update the compute descriptor set. No data was provided.");
    return;
  }

  const uint32_t totalQueuedBuffers = static_cast<uint32_t>(pJobInfo->pBufferAttachments.size()) + compute.freeBufferIndex;
  if (pJobInfo->pBufferAttachments.size() + compute.freeBufferIndex > compute.maxBoundDescriptorSets) {
    RE_LOG(Error, "Failed to update the compute descriptor set. The amount of buffers provided (%d) is"
      "higher than allowed for this frame (%d).",
      totalQueuedBuffers, compute.maxBoundDescriptorSets);
    return;
  }

  // Prepare tracking information for jobs in the current queue
  const uint32_t currentFreeImageIndex = compute.freeImageIndex;
  const uint32_t currentFreeSamplerIndex = compute.freeSamplerIndex;
  const uint32_t currentFreeBufferIndex = compute.freeBufferIndex;
  uint32_t totalImages = 0u;
  uint32_t totalSamplers = 0u;
  uint32_t totalBuffers = 0u;

  // Precalculate the number of required write descriptor sets
  const uint32_t imageWriteCount = static_cast<uint32_t>(pJobInfo->pImageAttachments.size());
  const uint32_t samplerWriteCount = static_cast<uint32_t>(pJobInfo->pSamplerAttachments.size());
  const uint32_t bufferWriteCount = static_cast<uint32_t>(pJobInfo->pBufferAttachments.size());
  uint32_t extraImageWriteCount = 0;
  uint32_t extraSamplerWriteCount = 0;

  if (pJobInfo->useExtraImageViews && imageWriteCount) {
    for (auto& image : pJobInfo->pImageAttachments) {
      extraImageWriteCount += static_cast<uint32_t>(image->texture.extraViews.size());
    }
  }

  if (pJobInfo->useExtraSamplerViews && samplerWriteCount) {
    for (auto& sampler : pJobInfo->pSamplerAttachments) {
      extraSamplerWriteCount += static_cast<uint32_t>(sampler->texture.extraViews.size());
    }
  }

  const uint32_t writeCount = imageWriteCount + samplerWriteCount
    + extraImageWriteCount + extraSamplerWriteCount + bufferWriteCount;

  if (writeCount > config::scene::storageImageBudget || writeCount == 0) {
    return;
  }

  // Update compute image set
  VkWriteDescriptorSet defaultWriteSet{};
  defaultWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  defaultWriteSet.dstSet = compute.imageDescriptorSet;
  defaultWriteSet.dstBinding = 0;
  defaultWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  defaultWriteSet.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets(writeCount, VkWriteDescriptorSet{});
  uint32_t arrayIndex = 0u;

  for (uint32_t imageIndex = 0; imageIndex < imageWriteCount; ++imageIndex) {
    writeDescriptorSets[arrayIndex] = defaultWriteSet;
    writeDescriptorSets[arrayIndex].dstArrayElement = currentFreeImageIndex + imageIndex;
    writeDescriptorSets[arrayIndex].pImageInfo = &pJobInfo->pImageAttachments[imageIndex]->texture.imageInfo;
    
    ++totalImages;
    ++arrayIndex;

    if (pJobInfo->useExtraImageViews && !pJobInfo->pImageAttachments[imageIndex]->texture.extraViews.empty()) {
      RTexture* pImage = pJobInfo->pImageAttachments[imageIndex];
      const uint32_t extraViewCount = static_cast<uint32_t>(pImage->texture.extraViews.size());

      for (uint32_t imageViewIndex = 0; imageViewIndex < extraViewCount; ++imageViewIndex) {
        pImage->texture.extraViews[imageViewIndex].imageLayout = pImage->texture.imageLayout;

        writeDescriptorSets[arrayIndex] = defaultWriteSet;
        writeDescriptorSets[arrayIndex].dstArrayElement = currentFreeImageIndex + imageIndex + imageViewIndex + 1;
        writeDescriptorSets[arrayIndex].pImageInfo = &pImage->texture.extraViews[imageViewIndex];

        ++totalImages;
        ++arrayIndex;
      }
    }
  }

  defaultWriteSet.dstBinding = 1;
  defaultWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  for (uint32_t samplerIndex = 0; samplerIndex < samplerWriteCount; ++samplerIndex) {
    writeDescriptorSets[arrayIndex] = defaultWriteSet;
    writeDescriptorSets[arrayIndex].dstArrayElement = samplerIndex + currentFreeSamplerIndex;
    writeDescriptorSets[arrayIndex].pImageInfo = &pJobInfo->pSamplerAttachments[samplerIndex]->texture.imageInfo;

    ++totalSamplers;
    ++arrayIndex;

    if (pJobInfo->useExtraSamplerViews && !pJobInfo->pSamplerAttachments[samplerIndex]->texture.extraViews.empty()) {
      RTexture* pSampler = pJobInfo->pSamplerAttachments[samplerIndex];
      const uint32_t extraViewCount = static_cast<uint32_t>(pSampler->texture.extraViews.size());

      for (uint32_t samplerViewIndex = 0; samplerViewIndex < extraViewCount; ++samplerViewIndex) {
        pSampler->texture.extraViews[samplerViewIndex].imageLayout = pSampler->texture.imageLayout;

        writeDescriptorSets[arrayIndex] = defaultWriteSet;
        writeDescriptorSets[arrayIndex].dstArrayElement = currentFreeSamplerIndex + samplerIndex + samplerViewIndex + 1;
        writeDescriptorSets[arrayIndex].pImageInfo = &pSampler->texture.extraViews[samplerViewIndex];

        ++totalSamplers;
        ++arrayIndex;
      }
    }
  }

  // Update compute buffer set
  defaultWriteSet.dstSet = compute.bufferDescriptorSet;
  defaultWriteSet.dstBinding = 0;
  defaultWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  std::vector<VkDescriptorBufferInfo> bufferInfo(bufferWriteCount);

  for (uint32_t bufferIndex = 0; bufferIndex < bufferWriteCount; ++bufferIndex) {
    bufferInfo[bufferIndex].buffer = pJobInfo->pBufferAttachments[bufferIndex]->buffer;
    bufferInfo[bufferIndex].offset = 0;
    bufferInfo[bufferIndex].range = VK_WHOLE_SIZE;

    writeDescriptorSets[arrayIndex] = defaultWriteSet;
    writeDescriptorSets[arrayIndex].dstBinding = bufferIndex;
    writeDescriptorSets[arrayIndex].pBufferInfo = &bufferInfo[bufferIndex];

    ++totalBuffers;
    ++arrayIndex;
  }

  vkUpdateDescriptorSets(core::renderer.logicalDevice.device, writeCount,
    writeDescriptorSets.data(), 0, nullptr);

  pJobInfo->pushBlock.imageIndex = compute.freeImageIndex;
  pJobInfo->pushBlock.samplerIndex = compute.freeSamplerIndex;
  pJobInfo->pushBlock.bufferIndex = compute.freeBufferIndex;

  compute.freeImageIndex += totalImages;
  compute.freeSamplerIndex += totalSamplers;
  compute.freeBufferIndex += totalBuffers;
}

void core::MRenderer::executeComputeJob(VkCommandBuffer commandBuffer, RComputeJobInfo* pJobInfo) {
  VkPipelineLayout layout = getPipelineLayout(EPipelineLayout::Compute);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, getComputePipeline(pJobInfo->pipeline));
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    layout, 0, 1, &compute.imageDescriptorSet, 0, nullptr);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    layout, 1, 1, &compute.bufferDescriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
    sizeof(RComputePCB), &pJobInfo->pushBlock);

  vkCmdDispatch(commandBuffer, pJobInfo->width, pJobInfo->height, pJobInfo->depth);

  if (pJobInfo->transtionToShaderReadOnly && system.enableLayoutTransitions) {
    VkImageSubresourceRange range{};
    range.baseArrayLayer = 0;
    range.baseMipLevel = 0;

    for (auto& image : pJobInfo->pImageAttachments) {
      range.aspectMask = image->texture.aspectMask;
      range.layerCount = image->texture.layerCount;
      range.levelCount = image->texture.levelCount;

      setImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
    }

    for (auto& sampler : pJobInfo->pSamplerAttachments) {
      range.layerCount = sampler->texture.layerCount;
      range.levelCount = sampler->texture.levelCount;

      setImageLayout(commandBuffer, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
    }
  }

  /*VkImageSubresourceRange range{};
  range.baseArrayLayer = 0;
  range.baseMipLevel = 0;

  for (auto& image : pJobInfo->pImageAttachments) {
    range.aspectMask = image->texture.aspectMask;
    range.layerCount = image->texture.layerCount;
    range.levelCount = image->texture.levelCount;

    setImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_GENERAL, range, true);
  }*/
}

void core::MRenderer::queueComputeJob(RComputeJobInfo* pInfo) {
  if (pInfo) {
    compute.queuedJobs.emplace_back(*pInfo);
  }
}

void core::MRenderer::preprocessQueuedComputeJobs(VkCommandBuffer commandBuffer) {
  compute.freeImageIndex = 0u;
  compute.freeSamplerIndex = 0u;
  compute.freeBufferIndex = 0u;

  for (auto& queuedJob : compute.queuedJobs) {
    VkImageSubresourceRange range{};
    range.baseArrayLayer = 0;
    range.baseMipLevel = 0;

    // Convert images if required, won't be converted if their layout is already valid
    for (auto& image : queuedJob.pImageAttachments) {
      range.aspectMask = image->texture.aspectMask;
      range.layerCount = image->texture.layerCount;
      range.levelCount = image->texture.levelCount;

      setImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_GENERAL, range);
    }

    for (auto& sampler : queuedJob.pSamplerAttachments) {
      range.aspectMask = sampler->texture.aspectMask;
      range.layerCount = sampler->texture.layerCount;
      range.levelCount = sampler->texture.levelCount;

      setImageLayout(commandBuffer, sampler, VK_IMAGE_LAYOUT_GENERAL, range);
    }

    // Iteratively update sets minding the offset into binding layout
    updateComputeDescriptorSets(&queuedJob);
  }
}

void core::MRenderer::executeQueuedComputeJobs(VkCommandBuffer commandBuffer) {
  if (compute.queuedJobs.empty()) return;

  beginCommandBuffer(commandBuffer, false);

  preprocessQueuedComputeJobs(commandBuffer);

  for (auto& queuedJob : compute.queuedJobs) {
    executeComputeJob(commandBuffer, &queuedJob);
  }

  flushCommandBuffer(commandBuffer, ECmdType::Compute);

  compute.queuedJobs.clear();
}