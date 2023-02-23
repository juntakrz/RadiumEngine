#pragma once

#include "common.h"

class ABase;

namespace core {
class MPlayer {
 private:
  ABase* m_pActor = nullptr;

 private:
  MPlayer();

  void bindDefaultMethods();

 public:
  static MPlayer& get() {
    static MPlayer _sInstance;
    return _sInstance;
  }

  // must be called after input manager is initialized
  void initialize();

  void controlActor(ABase* pActor);
  void controlCamera(const char* name);

  void freeActor(ABase* pActor);

  // standard movement methods which control currently selected actors
  void moveForward();
  void moveBack();
  void moveLeft();
  void moveRight();
  void moveUp();
  void moveDown();
};
}  // namespace core