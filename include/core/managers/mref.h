#pragma once

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
  T* getActor(const char* name) {
    return actorPointers.at(name).ptr;
  };

  template<>
  ABase* getActor<ABase>(const char* name) {
    if (actorPointers.at(name).type != EAType::BASE)
      RE_LOG(Warning,
             "Incorrect actor reference type. 'ABase' was requested but the "
             "type may be different.");

    return actorPointers.at(name).ptr;
  }

  template <>
  ACamera* getActor<ACamera>(const char* name) {
    if (actorPointers.at(name).type != EAType::CAMERA)
      RE_LOG(Warning,
             "Incorrect actor reference type. 'ACamera' was requested but the "
             "type may be different.");

    return dynamic_cast<ACamera*>(actorPointers.at(name).ptr);
  }
};