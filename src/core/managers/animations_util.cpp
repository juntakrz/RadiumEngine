#include "pch.h"
#include "util/util.h"
#include "core/managers/animations.h"


TResult core::MAnimations::loadAnimation(std::string filename,
                                         const std::string optionalNewName,
                                         const std::string skeleton) {
  /*if (filename.empty()) {
    RE_LOG(Error, "Failed to load animation, no filename was provided.");
    return RE_ERROR;
  }

  FileANM inFile;

  filename =
      RE_PATH_ANIMATIONS + skeleton + "/" + filename + RE_FEXT_ANIMATIONS;

  std::vector<char> inData = util::readFile(filename);

  // data must have at least the very basic header with 1 byte of animation name
  const int32_t minimalDataSize = 5 * sizeof(int32_t) + 1;

  if (inData.size() < minimalDataSize) {
    RE_LOG(Error,
           "Failed to load animation at '%s'. Unsupported format or data "
           "is corrupted.",
           filename.c_str());
    return RE_ERROR;
  }

  char* pData = inData.data();
  int32_t address = 0;

  // check for magic number
  int32_t magicNumber = *(int32_t*)&pData[address];
  address += sizeof(int32_t);

  if (magicNumber != RE_MAGIC_ANIMATIONS) {
    RE_LOG(Error, "Failed to load animation at '%s'. Unsupported format.",
           filename.c_str());
    return RE_ERROR;
  }

  inFile.headerBytes = *(int32_t*)&pData[address];
  address += sizeof(int32_t);

  inFile.dataBytes = *(int32_t*)&pData[address];
  address += sizeof(int32_t);

  if (inData.size() != inFile.headerBytes + inFile.dataBytes) {
    RE_LOG(Error,
           "Failed to load animation at '%s'. Data seems to be corrupted.",
           filename.c_str());
    return RE_ERROR;
  }

  inFile.animatedNodeCount = *(int32_t*)&pData[address];
  address += sizeof(int32_t);

  if (inFile.animatedNodeCount < 1) {
    RE_LOG(Warning, "Animation at '%s' has no data, won't be loaded.",
           filename.c_str());
  }

  inFile.framerate = *(float*)&pData[address];
  address += sizeof(float);

  inFile.duration = *(float*)&pData[address];
  address += sizeof(float);

  inFile.animationNameBytes = *(int32_t*)&pData[address];
  address += sizeof(int32_t);

  if (inFile.animationNameBytes < 1) {
    RE_LOG(Error,
           "Animation at '%s' has no name. Possible data corruption, won't be "
           "loaded.",
           filename.c_str());
  }

  inFile.animationName.resize(inFile.animationNameBytes);
  memcpy(inFile.animationName.data(), &pData[address],
         inFile.animationNameBytes);
  address += inFile.animationNameBytes;

  if (getAnimation(inFile.animationName) && optionalNewName.empty()) {
#ifndef NDEBUG
    RE_LOG(Warning,
           "Animation with the name '%s' already exists. It won't be replaced "
           "by loading '%s'.",
           inFile.animationName.c_str(), filename.c_str());
#endif
    return RE_WARNING;
  }

  inFile.nodeData.resize(inFile.animatedNodeCount);

  // fill in animated node structures
  for (int32_t i = 0; i < inFile.animatedNodeCount; ++i) {
    auto& nodeData = inFile.nodeData[i];
    
    nodeData.chunkBytes = *(int32_t*)&pData[address];
    address += sizeof(int32_t);

    nodeData.nodeIndex = *(int32_t*)&pData[address];
    address += sizeof(int32_t);

    nodeData.keyFrameCount = *(int32_t*)&pData[address];
    address += sizeof(int32_t);

    nodeData.jointCount = *(int32_t*)&pData[address];
    address += sizeof(int32_t);

    nodeData.nodeNameBytes = *(int32_t*)&pData[address];
    address += sizeof(int32_t);

    memcpy(nodeData.nodeName.data(), &pData[address], nodeData.nodeNameBytes);
    address += nodeData.nodeNameBytes;

    // fill in keyframes
    nodeData.keyFrameData.resize(nodeData.keyFrameCount);
    const int32_t nodeJointDataSize = nodeData.jointCount * sizeof(glm::mat4);

    for (int32_t j = 0; j < nodeData.keyFrameCount; ++j) {
      auto& keyFrame = nodeData.keyFrameData[j];

      keyFrame.timeStamp = *(float*)&pData[address];
      address += sizeof(float);

      keyFrame.nodeMatrix = *(glm::mat4*)&pData[address];
      address += sizeof(glm::mat4);

      if (nodeData.jointCount) {
        keyFrame.jointMatrices.resize(nodeData.jointCount);
        memcpy(keyFrame.jointMatrices.data(), &pData[address],
               nodeJointDataSize);
        address += nodeJointDataSize;
      }
    }
  }

  if (inData.size() != address) {
    RE_LOG(Error, "Parsing animation file '%s' failed.", filename.c_str());
    return RE_ERROR;
  }

  inData.clear();

  // create animation
  std::string animationName =
      (optionalNewName.empty()) ? inFile.animationName : optionalNewName;
  WAnimation* pAnimation = createAnimation(animationName);

  if (!pAnimation) {
    RE_LOG(Error, "Failed creating animation '%s' for '%s'.",
           animationName.c_str(), filename.c_str());
    return RE_ERROR;
  }

  pAnimation->m_framerate = inFile.framerate;
  pAnimation->m_duration = inFile.duration;

  for (const auto& inNode : inFile.nodeData) {
    pAnimation->addNodeReference(inNode.nodeName, inNode.nodeIndex);

    for (const auto& inKeyFrame : inNode.keyFrameData) {
      pAnimation->addKeyFrame(inNode.nodeIndex, inKeyFrame.timeStamp,
                              inKeyFrame.nodeMatrix, inKeyFrame.jointMatrices);
    }
  }

  return RE_OK;*/
}


TResult core::MAnimations::saveAnimation(const std::string animation,
                                         std::string filename,
                                         const std::string skeleton) {
  const WAnimation* pAnimation = getAnimation(animation);

  if (!pAnimation) {
    RE_LOG(Error, "Failed to save animation '%s'. Animation not found.",
           animation.c_str());
    return RE_ERROR;
  }

  if (filename.empty()) {
    filename = animation;
  }

  FileANM outFile;

  // header contains 5x 4 byte integers and 2x floats
  outFile.headerBytes = 5 * sizeof(int32_t) + 2 * sizeof(float);
  outFile.animatedNodeCount =
      static_cast<int32_t>(pAnimation->m_animatedNodes.size());

  outFile.framerate = pAnimation->m_framerate;
  outFile.duration = pAnimation->m_duration;

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
    //exportNodeData.keyFrameData = pAnimation->m_nodeKeyFrames.at(nodeIndex);
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

    const int32_t nodeJointDataSize =
        exportNodeData.jointCount * sizeof(glm::mat4);

    for (int32_t j = 0; j < exportNodeData.keyFrameCount; ++j) {
      const auto& keyFrame = exportNodeData.keyFrameData[j];
      chunkSize += sizeof(keyFrame.timeStamp);
      //chunkSize += sizeof(keyFrame.nodeMatrix);
      chunkSize += nodeJointDataSize;
    }

    chunkSize += sizeof(exportNodeData.chunkBytes);
    exportNodeData.chunkBytes = chunkSize;
    outFile.dataBytes += chunkSize;
  }

  int32_t totalBytes = outFile.headerBytes + outFile.dataBytes;
  char* pOutData = new char[totalBytes];
  int32_t address = 0;

  memcpy(&pOutData[address], &outFile, 5 * sizeof(int32_t) + 2 * sizeof(float));
  address += 5 * sizeof(int32_t) + 2 * sizeof(float);

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

    // add keyframe data
    for (int32_t l = 0; l < outFile.nodeData[k].keyFrameCount; ++l) {
      auto& keyFrame = outFile.nodeData[k].keyFrameData[l];

      memcpy(&pOutData[address], &keyFrame.timeStamp, sizeof(float));
      address += sizeof(float);

      //memcpy(&pOutData[address], &keyFrame.nodeMatrix, sizeof(glm::mat4));
      address += sizeof(glm::mat4);

      // add transformation matrices for joints
      for (int32_t m = 0; m < outFile.nodeData[k].jointCount; ++m) {
        memcpy(&pOutData[address], &keyFrame.jointMatrices[m],
               sizeof(glm::mat4));
        address += sizeof(glm::mat4);
      }
    }
  }

  if (totalBytes != address) {
    RE_LOG(Error, "Parsing animation '%s' for writing failed.",
           animation.c_str());
  }

  util::writeFile(filename + RE_FEXT_ANIMATIONS,
                  RE_PATH_ANIMATIONS + skeleton + "/", pOutData, totalBytes);

  delete[] pOutData;

  return RE_OK;
}