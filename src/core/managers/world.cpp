#include "pch.h"
#include "util/util.h"
#include "core/core.h"
#include "core/managers/materials.h"
#include "core/managers/world.h"
#include "core/world/model/primitive_plane.h"
#include "core/world/model/primitive_sphere.h"
//#include "core/world/model/primitive_cube.h"
#include "core/world/model/model.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

namespace core {
namespace callback {
bool loadImageData(tinygltf::Image* image, const int image_idx,
                   std::string* err, std::string* warn, int req_width,
                   int req_height, const unsigned char* bytes, int size,
                   void* user_data) {
  // empty callback - because only external textures are used
  return true;
}
}  // namespace callback
}  // namespace core

core::MWorld::MWorld() { RE_LOG(Log, "Initializing world manager."); }

TResult core::MWorld::loadModelFromFile(const std::string& path,
                                        std::string name) {
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF gltfContext;
  std::string error, warning;
  WModel* pModel = nullptr;
  bool bIsBinary = false, bIsModelLoaded = false;

  RE_LOG(Log, "Loading model \"%s\" from \"%s\".", name.c_str(), path.c_str());

  gltfContext.SetImageLoader(core::callback::loadImageData, nullptr);

  size_t extensionLocation = path.rfind(".", path.length());
  if (extensionLocation == std::string::npos) {
    RE_LOG(Error, "Failed to load model at \"%s\", incorrect path.",
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
    RE_LOG(Error,
           "Failed to load model at \"%s\". Warning: \"%s\", Error: \"%s\".",
           path.c_str(), warning.c_str(), error.c_str());
    return RE_ERROR;
  }

  // create the model object
  if (!m_models.try_emplace(name).second) {
    RE_LOG(
        Error,
        "Failed to add model \"%s\" to the world - a model with the same name "
        "already exists.",
        name.c_str());
    return RE_ERROR;
  }

  m_models.at(name) = std::make_unique<WModel>();
  pModel = m_models.at(name).get();

  pModel->m_name = name;

  uint32_t vertexCount = 0u, vertexPos = 0u, indexCount = 0u, indexPos = 0u;
  const tinygltf::Scene& gltfScene =
      gltfModel
          .scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

  // index of texture paths used by this model, required by the material setup later
  std::vector<std::string> texturePaths;

  // assign texture samplers for the current WModel
  pModel->setTextureSamplers(gltfModel);

  // go through glTF model's texture records
  for (const tinygltf::Texture& tex : gltfModel.textures) {
    const tinygltf::Image* pImage = &gltfModel.images[tex.source];

    // get custom corresponding sampler data from WModel if present
    RSamplerInfo textureSampler{};
    if (tex.sampler != -1) {
      textureSampler = pModel->m_textureSamplers[tex.sampler];
    }

    // add empty path, should be changed later
    texturePaths.emplace_back("");

    // get texture name and load it though materials manager
    if (pImage->uri == "") {
      continue;
    }

#ifndef NDEBUG
    RE_LOG(Log, "Loading texture \"%s\" for model \"%s\".", pImage->uri.c_str(),
           pModel->m_name.c_str());
#endif
    if (core::materials.loadTexture(pImage->uri.c_str(), &textureSampler) <
        RE_ERROR) {
      texturePaths.back() = pImage->uri;
    };
  }

  // get glTF materials and convert them to RMaterial
  pModel->parseMaterials(gltfModel, texturePaths);

  // parse node properties and get index/vertex counts
  for (size_t i = 0; i < gltfScene.nodes.size(); ++i) {
    pModel->parseNodeProperties(gltfModel, gltfModel.nodes[gltfScene.nodes[i]]);
  }

  // resize local staging buffers
  pModel->setLocalStagingBuffers();

  // create nodes using data from glTF model
  for (size_t n = 0; n < gltfScene.nodes.size(); ++n) {
    tinygltf::Node& gltfNode = gltfModel.nodes[gltfScene.nodes[n]];
    pModel->createNode(nullptr, gltfModel, gltfNode, gltfScene.nodes[n]);
  }

  // validate model staging buffers
  if (pModel->validateStagingBuffers() != RE_OK) {
    RE_LOG(Error,
           "Failed to create model \"%s\". Error when generating buffers.");
    return RE_ERROR;
  }
  
  if (gltfModel.animations.size() > 0) {
    pModel->loadAnimations(gltfModel);
  }
  pModel->loadSkins(gltfModel);

  for (auto node : pModel->m_pLinearNodes) {
    // Assign skins
    if (node->skinIndex > -1) {
      node->pSkin = pModel->m_pSkins[node->skinIndex].get();
    }
    // Initial pose
    if (node->pMesh) {
      node->updateNode();
    }

    // no need to update children, they are accessed anyway
    node->setNodeDescriptorSet(false);
  }

  return RE_OK;
}

TResult core::MWorld::createModel(EWPrimitive type, std::string name,
                                  int32_t arg0, int32_t arg1) {
  using RMaterial = core::MMaterials::RMaterial;
  auto fValidateNode = [&](WModel::Node* pNode) {
    if (pNode->pMesh == nullptr) {
      RE_LOG(Error, "Node validation failed for \"%s\", model: \"%s\".",
             pNode->name.c_str(), name.c_str());
      return RE_ERROR;
    }
    return RE_OK;
  };

  if (!m_models.try_emplace(name).second) {
    RE_LOG(Warning,
           "Failed to create model \"%s\". Similarly named model probably "
           "already exists.",
           name.c_str());
    return RE_WARNING;
  }

  m_models.at(name) = std::make_unique<WModel>();
  WModel* pModel = m_models.at(name).get();

  if (!pModel) {
    RE_LOG(Error, "Failed to create model \"%s\".", name.c_str());
    return RE_ERROR;
  }

  RE_LOG(Log, "Creating a primitive-based model \"%s\" (%d, %d).", name.c_str(),
         arg0, arg1);

  pModel->m_name = name;

  switch (type) {
    /*case EWPrimitive::Plane : {
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
    }*/

    default: {
      break;
    }
  }

  // assign default material to the model
  RMaterialInfo defaultMaterialInfo{};
  RMaterial* pDefaultMaterial =
      core::materials.createMaterial(&defaultMaterialInfo);

  for (auto& primitive : pModel->getPrimitives()) {
    primitive->pMaterial = pDefaultMaterial;
  }

  for (auto& node : pModel->getRootNodes()) {
    node->setNodeDescriptorSet(true);
    node->updateNode();
  }

  return RE_OK;
}

WModel* core::MWorld::getModel(const char* name) {
  if (m_models.contains(name)) {
    return m_models.at(name).get();
  }

  RE_LOG(Error, "Failed to get model \"%s\". It does not exist.", name);

  return nullptr;
}

void core::MWorld::destroyAllModels() {
  RE_LOG(Log, "Destroying all models.");

  for (auto& model : m_models) {
    std::unique_ptr<WModel>& pModel = model.second;
    std::string name = pModel->m_name;
    pModel->clean();
    pModel.reset();
    RE_LOG(Log, "Model \"%s\" was successfully destroyed.", name.c_str());
  }

  m_models.clear();
  
  RE_LOG(Log, "Finished removing models.");
}