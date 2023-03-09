#include "pch.h"
#include "core/core.h"
#include "core/managers/input.h"
#include "core/managers/player.h"
#include "core/managers/time.h"
#include "core/world/actors/base.h"

core::MPlayer::MPlayer() {
  RE_LOG(Log, "Created player controller manager.");
}

void core::MPlayer::bindDefaultMethods() {
  RE_LOG(Log, "Binding default player controller methods.");

  core::input.bindFunction(GETKEY("moveForward"), GLFW_PRESS, this,
                           &MPlayer::moveForward, true);
  core::input.bindFunction(GETKEY("moveBack"), GLFW_PRESS, this,
                           &MPlayer::moveBack, true);
  core::input.bindFunction(GETKEY("moveLeft"), GLFW_PRESS, this,
                           &MPlayer::moveLeft, true);
  core::input.bindFunction(GETKEY("moveRight"), GLFW_PRESS, this,
                           &MPlayer::moveRight, true);
  core::input.bindFunction(GETKEY("moveUp"), GLFW_PRESS, this, &MPlayer::moveUp,
                           true);
  core::input.bindFunction(core::MInput::get().bindingToKey("moveDown"),
                           GLFW_PRESS, this, &MPlayer::moveDown, true);
  
  core::input.bindFunction(GETKEY("rollLeft"), GLFW_PRESS, this,
                           &MPlayer::rollLeft, true);
  core::input.bindFunction(GETKEY("rollRight"), GLFW_PRESS, this,
                           &MPlayer::rollRight, true);
  core::input.bindFunction(GETKEY("pitchUp"), GLFW_PRESS, this,
                           &MPlayer::pitchUp, true);
  core::input.bindFunction(GETKEY("pitchDown"), GLFW_PRESS, this,
                           &MPlayer::pitchDown, true);
}

void core::MPlayer::initialize() { bindDefaultMethods(); }

void core::MPlayer::controlActor(ABase* pActor) {
  /* for (const auto it : m_inputActors) {
    if (it == pActor) {
      RE_LOG(Warning,
             "Actor at %d seems to be already receiving input, won't assign.",
             pActor);
      return;
    }
  }

  m_inputActors.emplace_back(pActor);*/

  m_pActor = pActor;
}

void core::MPlayer::controlCamera(const char* name) {}

void core::MPlayer::freeActor(ABase* pActor) {
  /*for (const auto it : m_inputActors) {
    if (it == pActor) {
      // m_inputActors.erase(it);
      return;
    }
  }

  RE_LOG(Warning, "Actor at %d is not receiving input, won't unassign.",
         pActor);*/
}

void core::MPlayer::moveForward() {
  m_pActor->translate({0.0f, 0.0f, m_movementData.translationDelta});
}

void core::MPlayer::moveBack() {
  m_pActor->translate({0.0f, 0.0f, -m_movementData.translationDelta});
}

void core::MPlayer::moveLeft() {
  m_pActor->translate({-m_movementData.translationDelta, 0.0f, 0.0f});
}

void core::MPlayer::moveRight() {
  m_pActor->translate({m_movementData.translationDelta, 0.0f, 0.0f});
}

void core::MPlayer::moveUp() {
  m_pActor->translate({0.0f, m_movementData.translationDelta, 0.0f});
}

void core::MPlayer::moveDown() {
  m_pActor->translate({0.0f, -m_movementData.translationDelta, 0.0f});
}

void core::MPlayer::rollLeft() {
  m_pActor->rotate({0.0f, 1.0f, 0.0f}, -m_movementData.rotationDelta);
}

void core::MPlayer::rollRight() {
  m_pActor->rotate({0.0f, 1.0f, 0.0f}, m_movementData.rotationDelta);
}

void core::MPlayer::pitchUp() {
  m_pActor->rotate({1.0f, 0.0f, 0.0f}, -m_movementData.rotationDelta);
}

void core::MPlayer::pitchDown() {
  m_pActor->rotate({1.0f, 0.0f, 0.0f}, m_movementData.rotationDelta);
}
