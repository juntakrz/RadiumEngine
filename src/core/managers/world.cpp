#include "pch.h"
#include "util/util.h"
#include "core/managers/world.h"
#include "core/world/model/primitive_plane.h"
#include "core/world/model/primitive_sphere.h"
//#include "core/world/model/primitive_cube.h"
#include "core/world/model/model.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf/tiny_gltf.h"

core::MWorld::MWorld() { RE_LOG(Log, "Initializing world manager."); }

TResult core::MWorld::loadModelFromFile(const std::string& path,
                                        std::string name) {
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF gltfContext;
  std::string error, warning;
  WModel* pModel = nullptr;
  bool bIsBinary = false, bIsModelLoaded = false;

  RE_LOG(Log, "Loading model '%s' from '%s'.", name.c_str(), path.c_str());

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

  // create the model object
  if (m_models.try_emplace(name).second) {
    m_models.at(name) = std::make_unique<WModel>();
    pModel = m_models.at(name).get();
  }

  uint32_t vertexCount = 0u, vertexPos = 0u, indexCount = 0u, indexPos = 0u;
  const tinygltf::Scene& gltfScene =
      gltfModel
          .scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

  // parse node properties and get index/vertex counts
  for (size_t i = 0; i < gltfScene.nodes.size(); ++i) {
    pModel->parseNodeProperties(gltfModel, gltfModel.nodes[gltfScene.nodes[i]]);
  }

  // create nodes using data from glTF model
  for (size_t n = 0; n < gltfScene.nodes.size(); ++n) {
    tinygltf::Node& gltfNode = gltfModel.nodes[gltfScene.nodes[n]];
    pModel->createNode(nullptr, gltfModel, gltfNode, gltfScene.nodes[n]);
  }

  /*
  if (gltfModel.animations.size() > 0) {
    loadAnimations(gltfModel);
  }
  loadSkins(gltfModel);*/

  return RE_OK;
}

TResult core::MWorld::createModel(EWPrimitive type, std::string name,
                                  int32_t arg0, int32_t arg1) {
  auto fValidateNode = [&](WModel::Node* pNode) {
    if (pNode->pMesh == nullptr) {
      RE_LOG(Error, "Node validation failed for '%s', model: '%s'.",
             pNode->name.c_str(), name.c_str());
      return RE_ERROR;
    }
    return RE_OK;
  };

  if (!m_models.try_emplace(name).second) {
    RE_LOG(Warning,
           "Failed to create model '%s'. Similarly named model probably "
           "already exists.",
           name.c_str());
    return RE_WARNING;
  }

  m_models.at(name) = std::make_unique<WModel>();
  WModel* pModel = m_models.at(name).get();

  if (!pModel) {
    RE_LOG(Error, "Failed to create model '%s'.", name.c_str());
    return RE_ERROR;
  }

  RE_LOG(Log, "Creating a primitive-based model '%s' (%d, %d).", name.c_str(),
         arg0, arg1);

  pModel->m_name = name;

  switch (type) {
    case EWPrimitive::Plane: {
      WModel::Node* pNode = pModel->createNode(nullptr, 0, "node_" + name);
      RE_CHECK(fValidateNode(pNode));
      pModel->m_pLinearNodes.emplace_back(pNode);
      pNode->pMesh->pPrimitives.emplace_back(
          std::make_unique<WPrimitive_Plane>());
      pNode->pMesh->pPrimitives.back()->create(arg0, arg1);
      pModel->m_pLinearPrimitives.emplace_back(
          pNode->pMesh->pPrimitives.back().get());
      break;
    }

    case EWPrimitive::Sphere: {
      WModel::Node* pNode = pModel->createNode(nullptr, 0, "node_" + name);
      RE_CHECK(fValidateNode(pNode));
      pModel->m_pLinearNodes.emplace_back(pNode);
      pNode->pMesh->pPrimitives.emplace_back(
          std::make_unique<WPrimitive_Sphere>());
      pNode->pMesh->pPrimitives.back()->create(arg0, arg1);
      pModel->m_pLinearPrimitives.emplace_back(
          pNode->pMesh->pPrimitives.back().get());
      break;
    }

    default: {
      break;
    }
  }

  return RE_OK;
}

WModel* core::MWorld::getModel(const char* name) {
  if (m_models.contains(name)) {
    return m_models.at(name).get();
  }

  RE_LOG(Error, "Failed to get model '%s'. It does not exist.", name);

  return nullptr;
}

void core::MWorld::destroyAllModels() {
  RE_LOG(Log, "Destroying all models.");

  for (auto& model : m_models) {
    std::unique_ptr<WModel>& pModel = model.second;
    std::string name = pModel->m_name;
    pModel->clean();
    pModel.reset();
    RE_LOG(Log, "Model '%s' was successfully destroyed.", name.c_str());
  }

  m_models.clear();
  
  RE_LOG(Log, "Finished removing models.");
}
