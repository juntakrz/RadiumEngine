#pragma once

#include "primitive.h"

class WPrimitive_Custom : public WPrimitive {
 public:
  WPrimitive_Custom(std::vector<RVertex>& vertexData,
                    const std::vector<uint32_t>& indexData);

  void create(std::vector<RVertex>& vertexData,
              const std::vector<uint32_t>& indexData);
};