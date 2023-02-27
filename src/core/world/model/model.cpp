#include "pch.h"
#include "core/core.h"
#include "core/managers/renderer.h"
#include "core/world/model/model.h"

glm::mat4 WModel::Node::getLocalMatrix() {
  return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) *
         glm::scale(glm::mat4(1.0f), scale) * matrix;
}

glm::mat4 WModel::Node::getMatrix() {
  glm::mat4 matrix = getLocalMatrix();
  Node* pParent = pParentNode;
  while (pParent) {
    matrix = pParent->getLocalMatrix() * matrix;
    pParent = pParent->pParentNode;
  }
  return matrix;
}

WModel::Node::Node() {
  // prepare buffer and memory to store node transformation matrix
  core::renderer.createBuffer(EBCMode::CPU_UNIFORM,
                              uniformBufferData.bufferSize,
                              uniformBufferData.uniformBuffer);
}

void WModel::Node::update() {
  if (!pMeshData) {
    glm::mat4 matrix = getMatrix();
    /*if (skin) {
      mesh->uniformBlock.matrix = m;
      // Update join matrices
      glm::mat4 inverseTransform = glm::inverse(m);
      size_t numJoints =
          std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);
      for (size_t i = 0; i < numJoints; i++) {
        vkglTF::Node* jointNode = skin->joints[i];
        glm::mat4 jointMat =
            jointNode->getMatrix() * skin->inverseBindMatrices[i];
        jointMat = inverseTransform * jointMat;
        mesh->uniformBlock.jointMatrix[i] = jointMat;
      }
      mesh->uniformBlock.jointcount = (float)numJoints;
      memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock,
             sizeof(mesh->uniformBlock));
    } else {
      memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
    }*/

    // store node matrix into a uniform buffer
    memcpy(uniformBufferData.uniformBuffer.allocInfo.pMappedData, &matrix,
           sizeof(glm::mat4));
  }

  for (auto& pChild : pChildren) {
    pChild->update();
  }
}
