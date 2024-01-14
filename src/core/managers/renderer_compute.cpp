#include "pch.h"
#include "core/core.h"
#include "core/managers/resources.h"
#include "core/managers/renderer.h"

void core::MRenderer::updateComputeImageSet(
  std::vector<RTexture*>* pInImages, const uint32_t imageOffset) {
  if (!pInImages || pInImages->size() > config::scene::storageImageBudget ||
    pInImages->size() < 1) {
    RE_LOG(Error,
      "Couldn't update compute image descriptor set. Invalid data was "
      "provided.");
    return;
  }

  compute.pImages = *pInImages;

  uint32_t writeSize = static_cast<uint32_t>(compute.pImages.size());
  std::vector<VkWriteDescriptorSet> writeDescriptorSets;
  writeDescriptorSets.resize(writeSize);

  for (uint32_t i = 0; i < writeSize; ++i) {
    writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[i].dstSet = compute.imageDescriptorSet;
    writeDescriptorSets[i].dstBinding = i;
    writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeDescriptorSets[i].descriptorCount = 1;
    writeDescriptorSets[i].pImageInfo = &compute.pImages[i]->texture.imageInfo;
  }

  vkUpdateDescriptorSets(core::renderer.logicalDevice.device, writeSize,
    writeDescriptorSets.data(), 0, nullptr);

  compute.imagePCB.imageIndex = imageOffset;
  compute.imagePCB.imageCount = writeSize;

  // the workgroup size is always determined by the leading image
  compute.imageExtent.width = compute.pImages[0]->texture.width;
  compute.imageExtent.height = compute.pImages[0]->texture.height;
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
    compute.imageExtent.height / core::vulkan::computeGroupCountY_2D, 1);
}

void core::MRenderer::createComputeJob(RComputeInfo* pInfo) {
  compute.jobs.emplace_back(*pInfo);
}

void core::MRenderer::executeComputeJobs() {
  RComputeInfo& pInfo = compute.jobs.front();

  switch (pInfo.jobType) {
    case EComputeJob::Image: {

      
      return;
    }
  }
}
