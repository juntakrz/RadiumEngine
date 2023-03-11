#include "pch.h"
#include "core/world/model/primitive_custom.h"

WPrimitive_Custom::WPrimitive_Custom(RPrimitiveInfo* pCreateInfo) {
  if (pCreateInfo->vertexCount < 3 || pCreateInfo->indexCount < 3) {
    RE_LOG(Error, "Invalid primitive creation data provided.");
    return;
  }

  vertexOffset = pCreateInfo->vertexOffset;
  vertexCount = pCreateInfo->vertexCount;
  indexOffset = pCreateInfo->indexOffset;
  indexCount = pCreateInfo->indexCount;

  isCustom = true;

  if (pCreateInfo->createTangentSpaceData) {
    if (!pCreateInfo->pVertexData || !pCreateInfo->pIndexData) {
      RE_LOG(Error,
             "Could not create tangent space data for vertex buffer - no "
             "valid data was provided.");
      return;
    }

    generateTangentsAndBinormals(*pCreateInfo->pVertexData,
                                 *pCreateInfo->pIndexData);
  }
}
