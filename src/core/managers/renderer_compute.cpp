#include "pch.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/time.h"
#include "core/managers/renderer.h"

void core::MRenderer::setDefaultComputeJobInfo() {
  RComputeJobInfo computeJob{};
  computeJob.jobType = EComputeJob::Image;
  computeJob.width = config::renderWidth / 2;
  computeJob.height = config::renderHeight / 2;
  computeJob.pipeline = EComputePipeline::ImagePPMipMap;
  computeJob.pImageAttachments = { core::resources.getTexture(RTGT_POSTPROCESS) };
  computeJob.intValues.x = computeJob.pImageAttachments[0]->texture.levelCount;
  computeJob.intValues.y = computeJob.width;
  computeJob.intValues.z = computeJob.height;
  computeJob.transtionToShaderReadOnly = true;

  postprocessing.computeJobs.ppMipMap = computeJob;
}

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

  const uint32_t totalWriteSize = imageWriteSize + samplerWriteSize + extraImageWriteSize + extraSamplerWriteSize;

  if (totalWriteSize > config::scene::storageImageBudget || totalWriteSize == 0) {
    return;
  }

  VkWriteDescriptorSet defaultWriteSet{};
  defaultWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  defaultWriteSet.dstSet = compute.imageDescriptorSet;
  defaultWriteSet.dstBinding = 0;
  defaultWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  defaultWriteSet.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets(totalWriteSize, VkWriteDescriptorSet{});
  uint32_t arrayIndex = 0u;

  for (uint32_t i = 0; i < imageWriteSize; ++i) {
    writeDescriptorSets[arrayIndex] = defaultWriteSet;
    writeDescriptorSets[arrayIndex].dstArrayElement = arrayIndex;
    writeDescriptorSets[arrayIndex].pImageInfo = &pInImages->at(i)->texture.imageInfo;
    
    arrayIndex++;

    if (useExtraImageViews && !pInImages->at(i)->texture.extraViews.empty()) {
      RTexture* pImage = pInImages->at(i);
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
      writeDescriptorSets[arrayIndex].pImageInfo = &pInSamplers->at(j)->texture.imageInfo;

      arrayIndex++;

      if (useExtraSamplerViews && !pInSamplers->at(j)->texture.extraViews.empty()) {
        RTexture* pSampler = pInSamplers->at(j);
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

  vkUpdateDescriptorSets(core::renderer.logicalDevice.device, totalWriteSize,
    writeDescriptorSets.data(), 0, nullptr);

  // TODO: Update this accounting for samplers also being bound now
  compute.imagePCB.imageIndex = 0;
  compute.imagePCB.imageCount = totalWriteSize;
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

void core::MRenderer::generateLUTMap() {
  RE_LOG(Log, "Generating BRDF LUT map to '%s' texture.", RTGT_LUTMAP);

  core::time.tickTimer();

  RTexture* pLUTTexture = core::resources.getTexture(RTGT_LUTMAP);
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
      &core::resources.getTexture(RTGT_LUTMAP)->texture.imageInfo;

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
  switch (pInfo->jobType) {
    case EComputeJob::Image: {
      VkCommandBuffer transitionBuffer = createCommandBuffer(ECmdType::Transfer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

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

        setImageLayout(transitionBuffer, image, VK_IMAGE_LAYOUT_GENERAL, range);
      }

      for (auto& sampler : pInfo->pSamplerAttachments) {
        range.layerCount = sampler->texture.layerCount;
        range.levelCount = sampler->texture.levelCount;

        setImageLayout(transitionBuffer, sampler, VK_IMAGE_LAYOUT_GENERAL, range);
      }

      flushCommandBuffer(transitionBuffer, ECmdType::Transfer, false, true);

      updateComputeImageSet(&pInfo->pImageAttachments, &pInfo->pSamplerAttachments, pInfo->useExtraImageViews, pInfo->useExtraSamplerViews);

      VkCommandBuffer cmdBuffer = createCommandBuffer(ECmdType::Compute, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
      executeComputeImage(cmdBuffer, pInfo->pipeline);
      flushCommandBuffer(cmdBuffer, ECmdType::Compute, true);

      beginCommandBuffer(transitionBuffer);

      if (pInfo->transtionToShaderReadOnly) {
        for (auto& image : pInfo->pImageAttachments) {
          range.layerCount = image->texture.layerCount;
          range.levelCount = image->texture.levelCount;

          setImageLayout(transitionBuffer, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
        }

        for (auto& sampler : pInfo->pSamplerAttachments) {
          range.layerCount = sampler->texture.layerCount;
          range.levelCount = sampler->texture.levelCount;

          setImageLayout(transitionBuffer, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
        }
      }

      flushCommandBuffer(transitionBuffer, ECmdType::Transfer, true);
      break;
    }
  }
}

void core::MRenderer::executeQueuedComputeJobs() {
  if (compute.jobs.empty()) return;

  RComputeJobInfo* pInfo = &compute.jobs.front();
  executeComputeJobImmediate(pInfo);
  compute.jobs.erase(compute.jobs.begin());
}