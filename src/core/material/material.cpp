#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/managers/resources.h"
#include "core/material/texture.h"
#include "core/material/material.h"

void RMaterial::createDescriptorSet() {
  // allocate material's descriptor set
  VkDescriptorSetLayout materialLayout =
      core::renderer.getDescriptorSetLayout(EDescriptorSetLayout::Material);

  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = core::renderer.getDescriptorPool();
  allocateInfo.pSetLayouts = &materialLayout;
  allocateInfo.descriptorSetCount = 1;

  if (vkAllocateDescriptorSets(core::renderer.logicalDevice.device,
                               &allocateInfo, &descriptorSet) != VK_SUCCESS) {
    RE_LOG(Error, "Failed to allocate descriptor set for the material \"%\".",
           name);
    return;
  };

  RTexture* pNullTexture = core::resources.getTexture(RE_WHITETEXTURE);

  if (!pNullTexture) {
    RE_LOG(Error,
           "Missing data. Materials manager was not initialized correctly.");
    return;
  }

  // retrieve all material's texture image descriptors
  std::vector<VkDescriptorImageInfo> imageDescriptors = {
      pBaseColor ? pBaseColor->texture.descriptor
                 : pNullTexture->texture.descriptor,
      pNormal ? pNormal->texture.descriptor : pNullTexture->texture.descriptor,
      pMetalRoughness ? pMetalRoughness->texture.descriptor
                      : pNullTexture->texture.descriptor,
      pOcclusion ? pOcclusion->texture.descriptor
                 : pNullTexture->texture.descriptor,
      pEmissive ? pEmissive->texture.descriptor
                : pNullTexture->texture.descriptor,
      pExtra ? pExtra->texture.descriptor : pNullTexture->texture.descriptor,
  };

  // write retrieved data to newly allocated descriptor set
  std::vector<VkWriteDescriptorSet> writeDescriptorSets;
  uint32_t writeSize = static_cast<uint32_t>(imageDescriptors.size());
  writeDescriptorSets.resize(writeSize);

  for (uint32_t i = 0; i < writeSize; ++i) {
    // if an image is a color attachment - it's probably a render target that
    // will be used as a shader data source at some point
    if (imageDescriptors[i].imageLayout ==
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
      imageDescriptors[i].imageLayout =
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[i].dstSet = descriptorSet;
    writeDescriptorSets[i].dstBinding = i;
    writeDescriptorSets[i].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSets[i].descriptorCount = 1;
    writeDescriptorSets[i].pImageInfo = &imageDescriptors[i];
  }

  vkUpdateDescriptorSets(core::renderer.logicalDevice.device, writeSize,
                         writeDescriptorSets.data(), 0, nullptr);
}