#include "pch.h"
#include "core/managers/world.h"

#include <tinygltf/tiny_gltf.h>

core::MWorld::MWorld() { RE_LOG(Log, "Initializing world manager."); }

void core::MWorld::processModelNode(WModel* pNewModel,
                                    const tinygltf::Model& model,
                                    const tinygltf::Node& node) {
  if (!pNewModel) {
    RE_LOG(Error, "%s: no target model provided.", __FUNCTION__);
    return;
  }

  if (node.children.size() > 0) {
    for (size_t i = 0; i < node.children.size(); i++) {
      processModelNode(pNewModel, model, model.nodes[node.children[i]]);
    }
  }


}

TResult core::MWorld::loadModelFromFile(const std::string& path,
                                        std::string name) {
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF gltfContext;
  std::string error, warning;
  WModel* pModel = nullptr;
  bool bIsBinary = false, bIsModelLoaded = false;

  size_t extensionLocation = path.rfind(".", path.length());
  if (extensionLocation == std::string::npos) {
    RE_LOG(Error, "Failed to load model at '%s', incorrect path.",
           path.c_str());
    return RE_ERROR;
  }

  bIsBinary = (path.substr(extensionLocation + 1,
                           path.length() - extensionLocation) == "glb");
  bIsModelLoaded =
      (bIsBinary)
          ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, path)
          : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, path);

  if (!bIsModelLoaded) {
    RE_LOG(Error, "Failed loading model at '%s'. Warning: '%s', Error: '%s'.",
           path.c_str(), warning.c_str(), error.c_str());
    return RE_ERROR;
  }

  // now it's time to create the model object
  if (m_models.try_emplace(name).second) {
    m_models.at(name) = std::make_unique<WModel>();
    pModel = m_models.at(name).get();
  }

  uint32_t vertexCount = 0u, vertexPos = 0u, indexCount = 0u, indexPos = 0u;
  const tinygltf::Scene gltfScene =
      gltfModel
          .scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

  for (size_t i = 0; i < gltfScene.nodes.size(); ++i) {
    processModelNode(pModel, gltfModel, gltfModel.nodes[gltfScene.nodes[i]]);
  }

  return RE_OK;
}