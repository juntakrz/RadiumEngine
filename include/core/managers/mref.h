#pragma once

#include "core/objects.h"
#include "common.h"

class MRef {

  std::unordered_map<std::string, WActorPtr> actorPointers;

 private:
  MRef();

 public:
  static MRef& get() {
    static MRef _sInstance;
    return _sInstance;
  }

  MRef(const MRef&) = delete;
  MRef& operator=(const MRef&) = delete;

  void registerActor(const char* name, class ABase* pActor, EAType type);

  template<typename T>
  T* getActor(const char* name){};
};