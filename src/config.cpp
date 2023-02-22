#include "config.h"

const char* config::appTitle = "Radium Engine";
const char* config::engineTitle = "Radium Engine";
uint32_t config::renderWidth = 1280;
uint32_t config::renderHeight = 720;
float config::viewDistance = 1000.0f;
float config::FOV = 90.0f;
bool config::bDevMode = false;
float config::pitchLimit = glm::radians(88.0f);

float config::getAspectRatio() { return renderWidth / (float)renderHeight; }
