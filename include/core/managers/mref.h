#pragma once

#include "common.h"

class ABase;

namespace core {

class MRef {
 private:
  std::unordered_map<std::string, ABase*> m_actorPointers;

 private:
  MRef();

 public:
  static MRef& get() {
    static MRef _sInstance;
    return _sInstance;
  }

  MRef(const MRef&) = delete;
  MRef& operator=(const MRef&) = delete;

  void registerActor(const char* name, class ABase* pActor);

  ABase* getActor(const char* name);
};
}  // namespace core