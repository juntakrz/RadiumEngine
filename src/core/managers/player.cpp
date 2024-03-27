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
  
  core::input.bindFunction(GETKEY("yawLeft"), GLFW_PRESS, this,
                           &MPlayer::yawLeft, true);
  core::input.bindFunction(GETKEY("yawRight"), GLFW_PRESS, this,
                           &MPlayer::yawRight, true);
  core::input.bindFunction(GETKEY("pitchUp"), GLFW_PRESS, this,
                           &MPlayer::pitchUp, true);
  core::input.bindFunction(GETKEY("pitchDown"), GLFW_PRESS, this,
                           &MPlayer::pitchDown, true);

  core::input.bindFunctionToMouseAxis(this, &MPlayer::yawMouse, false);
  core::input.bindFunctionToMouseAxis(this, &MPlayer::pitchMouse, true);
}

void core::MPlayer::initialize() { bindDefaultMethods(); }

void core::MPlayer::controlActor(ABase* pActor) {
  m_pActor = pActor;
  m_pActor->onPossessed();
}

void core::MPlayer::freeActor(ABase* pActor) {
  if (!m_pActor) {
    RE_LOG(Error, "No actor is currently possesed.");
    return;
  }
  
  // Code to unpossess currently controlled actor
}

void core::MPlayer::moveForward() {
  m_pActor->onControllerMovement(
    glm::vec3(0.0f, 0.0f, m_movementData.translationDelta * core::time.getDeltaTime()), false);
}

void core::MPlayer::moveBack() {
  m_pActor->onControllerMovement(
    glm::vec3(0.0f, 0.0f, -m_movementData.translationDelta * core::time.getDeltaTime()), false);
}

void core::MPlayer::moveLeft() {
  m_pActor->onControllerMovement(
    glm::vec3(-m_movementData.translationDelta * core::time.getDeltaTime(), 0.0f, 0.0f), false);
}

void core::MPlayer::moveRight() {
  m_pActor->onControllerMovement(
    glm::vec3(m_movementData.translationDelta * core::time.getDeltaTime(), 0.0f, 0.0f), false);
}

void core::MPlayer::moveUp() {
  m_pActor->onControllerMovement(
    glm::vec3(0.0f, m_movementData.translationDelta * core::time.getDeltaTime(), 0.0f), false);
}

void core::MPlayer::moveDown() {
  m_pActor->onControllerMovement(
    glm::vec3(0.0f, -m_movementData.translationDelta * core::time.getDeltaTime(), 0.0f), false);
}

void core::MPlayer::yawLeft() {
  m_pActor->onControllerMovement(
    glm::vec3(0.0f, -m_movementData.rotationDelta * core::time.getDeltaTime(), 0.0f), true);
}

void core::MPlayer::yawRight() {
  m_pActor->onControllerMovement(
    glm::vec3(0.0f, m_movementData.rotationDelta * core::time.getDeltaTime(), 0.0f), true);
}

void core::MPlayer::yawMouse() {
  switch (core::input.getControlMode()) {
    case EControlMode::MouseLook: {
        float yawDelta = core::input.getMouseDelta().x * core::time.getDeltaTime();
        m_pActor->onControllerMovement(glm::vec3(0.0f, yawDelta, 0.0f), true);
        break;
    }
  }
}

void core::MPlayer::pitchUp() {
  m_pActor->setRotation(glm::vec3(-m_movementData.rotationDelta * core::time.getDeltaTime(), 0.0f, 0.0f), true, true);
}

void core::MPlayer::pitchDown() {
  m_pActor->setRotation(glm::vec3(m_movementData.rotationDelta * core::time.getDeltaTime(), 0.0f, 0.0f), true, true);
}

void core::MPlayer::pitchMouse() {
  switch (core::input.getControlMode()) {
    case EControlMode::MouseLook: {
      float pitchDelta = core::input.getMouseDelta().y * core::time.getDeltaTime();
      m_pActor->onControllerMovement(glm::vec3(-pitchDelta, 0.0f, 0.0f), true);
    }
  }
}
