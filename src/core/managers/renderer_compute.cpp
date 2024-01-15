#include "pch.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/time.h"
#include "core/managers/renderer.h"

void core::MRenderer::updateComputeImageSet(std::vector<RTexture*>* pInImages, const uint32_t imageOffset) {
  if (!pInImages) {
    RE_LOG(Error, "Failed to update compute image descriptor set. No data was provided.");
    return;
  }

  const uint32_t writeSize = static_cast<uint32_t>(pInImages->size());

  if (writeSize > config::scene::storageImageBudget || writeSize == 0) {
    RE_LOG(Error,
      "Couldn't update compute image descriptor set. Invalid data was "
      "provided.");
    return;
  }

  std::vector<VkWriteDescriptorSet> writeDescriptorSets;
  writeDescriptorSets.resize(writeSize);

  for (uint32_t i = 0; i < writeSize; ++i) {
    writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[i].dstSet = compute.imageDescriptorSet;
    writeDescriptorSets[i].dstBinding = 0;
    writeDescriptorSets[i].dstArrayElement = i;
    writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeDescriptorSets[i].descriptorCount = 1;
    writeDescriptorSets[i].pImageInfo = &pInImages->at(i)->texture.imageInfo;
  }

  vkUpdateDescriptorSets(core::renderer.logicalDevice.device, writeSize,
    writeDescriptorSets.data(), 0, nullptr);

  compute.imagePCB.imageIndex = imageOffset;
  compute.imagePCB.imageCount = writeSize;
}

void core::MRenderer::updateComputeImageSet(std::vector<VkImageView>* pInViews, const uint32_t imageOffset) {
  const uint32_t viewSize = static_cast<uint32_t>(pInViews->size());

  if (!pInViews || viewSize > config::scene::storageImageBudget || viewSize == 0) {
    RE_LOG(Error,
      "Couldn't update compute image descriptor set. Invalid data was "
      "provided.");
    return;
  }

  std::vector<VkDescriptorImageInfo> imageInfo(viewSize);
  std::vector<VkWriteDescriptorSet> writeDescriptorSets(viewSize);

  for (uint32_t i = 0; i < viewSize; ++i) {
    imageInfo[i].imageView = pInViews->at(i);
  }

  for (uint32_t j = 0; j < viewSize; ++j) {
    writeDescriptorSets[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[j].dstSet = compute.imageDescriptorSet;
    writeDescriptorSets[j].dstBinding = j;
    writeDescriptorSets[j].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeDescriptorSets[j].descriptorCount = 1;
    writeDescriptorSets[j].pImageInfo = &imageInfo[j];
  }

  vkUpdateDescriptorSets(core::renderer.logicalDevice.device, viewSize,
    writeDescriptorSets.data(), 0, nullptr);

  compute.imagePCB.imageIndex = imageOffset;
  compute.imagePCB.imageCount = viewSize;
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

  vkCmdDispatch(
    commandBuffer,
    compute.imageExtent.width / core::vulkan::computeGroupCountX_2D,
    compute.imageExtent.height / core::vulkan::computeGroupCountY_2D,
    compute.imageExtent.depth);
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

void core::MRenderer::createComputeJob(RComputeJobInfo* pInfo) {
  if (pInfo) {
    if (pInfo->depth == 0) pInfo->depth = 1;

    compute.jobs.emplace_back(*pInfo);
  }
}

void core::MRenderer::executeComputeJobs() {
  if (compute.jobs.empty()) return;

  RComputeJobInfo& info = compute.jobs.front();

  switch (info.jobType) {
    case EComputeJob::Image: {
      VkCommandBuffer transitionBuffer = createCommandBuffer(ECmdType::Graphics, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
      VkImageSubresourceRange range{};
      range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      range.baseArrayLayer = 0;
      range.baseMipLevel = 0;

      compute.imageExtent.width = info.width;
      compute.imageExtent.height = info.height;
      compute.imageExtent.depth = info.depth;

      for (auto& it : info.pImageAttachments) {
        range.layerCount = it->texture.layerCount;
        range.levelCount = it->texture.levelCount;

        setImageLayout(transitionBuffer, it, VK_IMAGE_LAYOUT_GENERAL, range);
      }

      updateComputeImageSet(&info.pImageAttachments, 0);

      VkCommandBuffer cmdBuffer = createCommandBuffer(ECmdType::Compute, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
      executeComputeImage(cmdBuffer, info.pipeline);
      flushCommandBuffer(cmdBuffer, ECmdType::Compute, true);

      if (info.transtionToShaderReadOnly) {
        for (auto& it : info.pImageAttachments) {
          range.layerCount = it->texture.layerCount;
          range.levelCount = it->texture.levelCount;

          setImageLayout(transitionBuffer, it, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
        }
      }
      flushCommandBuffer(transitionBuffer, ECmdType::Graphics, true);

      compute.jobs.erase(compute.jobs.begin());
      return;
    }
  }
}