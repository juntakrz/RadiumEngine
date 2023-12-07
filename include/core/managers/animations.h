#pragma once

#include "core/model/animation.h"

namespace core {
class MAnimations {
 private:
  std::unordered_map<std::string, std::unique_ptr<WAnimation>> m_animations;

  MAnimations(){};

 public:
  static MAnimations& get() {
    static MAnimations _sInstance;
    return _sInstance;
  }

  MAnimations(const MAnimations&) = delete;
  MAnimations& operator=(const MAnimations&) = delete;

  WAnimation* createAnimation(const std::string& inName,
                             const int32_t inFramerate = 15);
  void removeAnimation(const std::string& inName);
  WAnimation* getAnimation(const std::string& inName);
  void clearAllAnimations();
};
}