#include "pch.h"
#include "core/core.h"
#include "core/managers/input.h"
#include "core/managers/actors.h"
#include "core/managers/renderer.h"

void core::MRenderer::debug_initialize() {
  core::input.bindFunction(GETKEY("F1"), GLFW_RELEASE, this,
                           &MRenderer::debug_viewMainCamera);
  core::input.bindFunction(GETKEY("F2"), GLFW_RELEASE, this,
                           &MRenderer::debug_viewSunCamera);
}

void core::MRenderer::debug_viewMainCamera() {
  RE_LOG(Log, "Viewing main camera.");
  setCamera(core::actors.getCamera(RCAM_MAIN));
}
void core::MRenderer::debug_viewSunCamera() {
  RE_LOG(Log, "Viewing sun camera.");
  setCamera(core::actors.getCamera(RCAM_SUN));
}
