#include "pch.h"
#include "util/util.h"
#include "core/core.h"
#include "core/managers/materials.h"
#include "core/managers/world.h"
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
  TResult result;

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

  result = pModel->createModel(name.c_str(), &gltfModel);

  return result;
}

TResult core::MWorld::createModel(EPrimitiveType type, std::string name,
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
  WModel::Node* pNode = pModel->createNode(nullptr, 0, "node_" + name);
  RE_CHECK(fValidateNode(pNode));
  pModel->m_pLinearNodes.emplace_back(pNode);

  RPrimitiveInfo primitiveInfo{};
  primitiveInfo.vertexOffset = pModel->staging.currentVertexOffset;
  primitiveInfo.indexOffset = pModel->staging.currentIndexOffset;
  primitiveInfo.createTangentSpaceData = true;

  pNode->pMesh->pPrimitives.emplace_back(
      std::make_unique<WPrimitive>(&primitiveInfo));

  std::vector<RVertex> vertices;
  std::vector<uint32_t> indices;

  switch (type) {
    case EPrimitiveType::Plane: {
      pNode->pMesh->pPrimitives.back()->generatePlane(arg0, arg1, vertices,
                                                      indices);
      break;
    }

    case EPrimitiveType::Sphere: {
      pNode->pMesh->pPrimitives.back()->generateSphere(arg0, arg1, vertices,
                                                       indices);
      break;
    }

    case EPrimitiveType::Cube: {
      pNode->pMesh->pPrimitives.back()->generateCube(arg0, arg1, vertices,
                                                     indices);
      break;
    }

    default: {
      break;
    }
  }

  // copy vertex and index data to local staging buffers and adjust offsets
  pModel->staging.vertices.insert(pModel->staging.vertices.begin(), vertices.begin(), vertices.end());
  pModel->staging.indices.insert(pModel->staging.indices.begin(), indices.begin(), indices.end());

  pModel->staging.currentVertexOffset += static_cast<uint32_t>(vertices.size());
  pModel->staging.currentIndexOffset += static_cast<uint32_t>(indices.size());
  pModel->m_vertexCount += pModel->staging.currentVertexOffset;
  pModel->m_indexCount += pModel->staging.currentIndexOffset;

  pModel->m_pLinearPrimitives.emplace_back(pNode->pMesh->pPrimitives.back().get());

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

  // calculate bounding box extent for the whole mesh based on created primitives
  /*
  glm::vec3 minExtent{0.0f}, maxExtent{0.0f};
  for (const auto& primitive : pNode->pMesh->pPrimitives) {
    if (primitive->getBoundingBoxExtent(minExtent, maxExtent)) {
      pNode->pMesh->extent.min = glm::min(pNode->pMesh->extent.min, minExtent);
      pNode->pMesh->extent.max = glm::max(pNode->pMesh->extent.max, maxExtent);
      pNode->pMesh->extent.isValid = true;

      pModel->m_pLinearPrimitives.emplace_back(primitive.get());
    }
  }*/

  return pModel->createStagingBuffers();
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