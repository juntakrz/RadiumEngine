#pragma once

#include "common.h"
#include "core/objects.h"
#include "util/math.h"

class WActor;

namespace core {
class MPlayer {
 private:
  WActor* m_pActor = nullptr;
  WPlayerInfo m_info;

  struct {
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

  void controlActor(WActor* pActor);
  void freeActor(WActor* pActor);
  void setActorControlMode(EActorControlMode newMode);

  const WPlayerInfo& getProperties();

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
  void rollLeft();
  void rollRight();
};
}  // namespace core