#include "pch.h"
#include "core/core.h"
#include "core/managers/minput.h"
#include "core/managers/mplayer.h"
#include "core/world/actors/abase.h"

core::MPlayer::MPlayer() {
  RE_LOG(Log, "Created player controller manager.");
}

void core::MPlayer::bindDefaultMethods() {
  RE_LOG(Log, "Binding default player controller methods.");

  core::input.bindFunction(GETKEY("moveForward"), GLFW_PRESS, this,
                           &MPlayer::moveForward);
  core::input.bindFunction(GETKEY("moveBack"), GLFW_PRESS, this,
                           &MPlayer::moveBack);
  core::input.bindFunction(GETKEY("moveLeft"), GLFW_PRESS, this,
                           &MPlayer::moveLeft);
  core::input.bindFunction(GETKEY("moveRight"), GLFW_PRESS, this,
                           &MPlayer::moveRight);
  core::input.bindFunction(GETKEY("moveUp"), GLFW_PRESS, this,
                           &MPlayer::moveUp);
  core::input.bindFunction(GETKEY("moveDown"), GLFW_PRESS, this,
                           &MPlayer::moveDown);
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
  m_pActor->translate(0.0f, 0.0f, 1.0f);
}

void core::MPlayer::moveBack() {
  m_pActor->translate(0.0f, 0.0f, -1.0f);
}

void core::MPlayer::moveLeft() {
  m_pActor->translate(-1.0f, 0.0f, 0.0f);
}

void core::MPlayer::moveRight() {
  m_pActor->translate(1.0f, 0.0f, 0.0f);
}

void core::MPlayer::moveUp() {
  m_pActor->translate(0.0f, 1.0f, 0.0f);
}

void core::MPlayer::moveDown() {
  m_pActor->translate(0.0f, -1.0f, 0.0f);
}
