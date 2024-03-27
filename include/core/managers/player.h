#pragma once

#include "common.h"
#include "util/math.h"

class ABase;

namespace core {
class MPlayer {
 private:
  ABase* m_pActor = nullptr;

  struct MovementData {
    float translationDelta = 1.0f;
    float rotationDelta = 1.0f;
  } m_movementData;

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
  void freeActor(ABase* pActor);

  MovementData& getMovementData() { return m_movementData; };

  // standard movement methods which control currently selected actors
  void moveForward();
  void moveBack();
  void moveLeft();
  void moveRight();
  void moveUp();
  void moveDown();

  void yawLeft();
  void yawRight();
  void yawMouse();
  void pitchUp();
  void pitchDown();
  void pitchMouse();
};
}  // namespace core