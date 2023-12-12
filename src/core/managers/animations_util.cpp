#include "pch.h"
#include "util/util.h"
#include "core/managers/animations.h"

TResult core::MAnimations::saveAnimation(const std::string& animation,
                                         const std::string& filename,
                                         const std::string skeleton) {
  FileANM outFile;
  const WAnimation* pAnimation = getAnimation(animation);

  if (!pAnimation) {
    RE_LOG(Error, "Failed to save animation '%s'. Not found.",
           animation.c_str());
    return RE_ERROR;
  }

  // header contains 5x 4 byte integers
  outFile.headerBytes = 5 * sizeof(int32_t);
  outFile.animatedNodeCount =
      static_cast<int32_t>(pAnimation->m_animatedNodes.size());

  outFile.animationName = animation;
  outFile.animationNameBytes =
      static_cast<int32_t>(outFile.animationName.size());
  outFile.headerBytes += outFile.animationNameBytes;
  outFile.dataBytes = 0;

  for (int32_t i = 0; i < outFile.animatedNodeCount; ++i) {
    outFile.nodeData.emplace_back();
    auto& exportNodeData = outFile.nodeData.back();
    const int32_t nodeIndex = pAnimation->m_animatedNodes[i].index;
    int32_t chunkSize = 0;

    exportNodeData.nodeIndex = nodeIndex;
    exportNodeData.keyFrameData = pAnimation->m_nodeKeyFrames.at(nodeIndex);
    exportNodeData.keyFrameCount =
        static_cast<int32_t>(exportNodeData.keyFrameData.size());
    exportNodeData.jointCount = static_cast<int32_t>(
        exportNodeData.keyFrameData.at(0).jointMatrices.size());

    chunkSize += sizeof(exportNodeData.nodeIndex);
    chunkSize += sizeof(exportNodeData.keyFrameCount);
    chunkSize += sizeof(exportNodeData.jointCount);

    exportNodeData.nodeName = pAnimation->m_animatedNodes[i].name;
    exportNodeData.nodeNameBytes =
        static_cast<int32_t>(exportNodeData.nodeName.size());
    chunkSize += sizeof(exportNodeData.nodeNameBytes);
    chunkSize += exportNodeData.nodeNameBytes;

    for (int32_t j = 0; j < exportNodeData.keyFrameCount; ++j) {
      const auto& keyFrame = exportNodeData.keyFrameData[j];
      chunkSize += sizeof(keyFrame.timeStamp);
      chunkSize += sizeof(keyFrame.nodeMatrix);
      chunkSize += sizeof(exportNodeData.jointCount * sizeof(glm::mat4));
    }

    chunkSize += sizeof(exportNodeData.chunkBytes);
    exportNodeData.chunkBytes = chunkSize;
    outFile.dataBytes += chunkSize;
  }

  int32_t totalBytes = outFile.headerBytes + outFile.dataBytes;
  char* pOutData = new char[totalBytes];
  int32_t address = 0;

  memcpy(&pOutData[address], &outFile, 5 * sizeof(int32_t));
  address += 5 * sizeof(int32_t);

  memcpy(&pOutData[address], outFile.animationName.c_str(),
         outFile.animationNameBytes);
  address += outFile.animationNameBytes;

  // node chunk header top part size contains 5x 4 byte integers
  const int32_t chunkTopSize = 5 * sizeof(int32_t);

  for (int32_t k = 0; k < outFile.animatedNodeCount; ++k) {
    memcpy(&pOutData[address], &outFile.nodeData[k], chunkTopSize);
    address += chunkTopSize;

    memcpy(&pOutData[address], outFile.nodeData[k].nodeName.c_str(),
           outFile.nodeData[k].nodeNameBytes);
    address += outFile.nodeData[k].nodeNameBytes;

    const int32_t chunkHeaderSize =
        chunkTopSize + outFile.nodeData[k].nodeNameBytes;

    const int32_t chunkDataSize =
        outFile.nodeData[k].chunkBytes - chunkHeaderSize;
    memcpy(&pOutData[address], outFile.nodeData[k].keyFrameData.data(),
           chunkDataSize);
    address += chunkDataSize;
  }

  if (totalBytes != address) {
    RE_LOG(Error, "Parsing animation '%s' for writing failed.",
           animation.c_str());
  }

  util::writeFile(animation + RE_FEXT_ANIMATIONS,
                  RE_PATH_ANIMATIONS + skeleton + "/", pOutData, totalBytes);

  delete[] pOutData;

  return RE_OK;
}