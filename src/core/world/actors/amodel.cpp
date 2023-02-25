#include "pch.h"
#include "core/world/actors/amodel.h"

#include <tinygltf/tiny_gltf.h>

glm::mat4 AModel::SModelNode::getLocalMatrix() { return glm::mat4(); }

glm::mat4 AModel::SModelNode::getMatrix() { return glm::mat4(); }

void AModel::SModelNode::update() {}
