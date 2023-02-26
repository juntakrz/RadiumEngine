#include "pch.h"
#include "core/world/model/model.h"

glm::mat4 WModel::ModelNode::getLocalMatrix() {
  return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) *
         glm::scale(glm::mat4(1.0f), scale) * matrix;
}

glm::mat4 WModel::ModelNode::getMatrix() {
  glm::mat4 m = getLocalMatrix();
  /*vkglTF::Node *p = parent;
  while (p) {
    m = p->localMatrix() * m;
    p = p->parent;
  }*/
  return m;
}

void WModel::ModelNode::update() {}
