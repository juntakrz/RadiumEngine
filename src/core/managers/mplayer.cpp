#include "pch.h"
#include "core/core.h"
#include "core/managers/minput.h"
#include "core/managers/mplayer.h"

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
  RE_LOG(Log, "%s: Moving forward.", __FUNCTION__);
}

void core::MPlayer::moveBack() {
  RE_LOG(Log, "%s: Moving back.", __FUNCTION__);
}

void core::MPlayer::moveLeft() {
  RE_LOG(Log, "%s: Moving left.", __FUNCTION__);
}

void core::MPlayer::moveRight() {
  RE_LOG(Log, "%s: Moving right.", __FUNCTION__);
}

void core::MPlayer::moveUp() {
  RE_LOG(Log, "%s: Moving up.", __FUNCTION__);
}

void core::MPlayer::moveDown() {
  RE_LOG(Log, "%s: Moving down.", __FUNCTION__);
}
