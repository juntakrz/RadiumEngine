#include "config.h"

const char* config::appTitle = "Radium Engine";
const char* config::engineTitle = "Radium Engine";
uint32_t config::renderWidth = 1280;
uint32_t config::renderHeight = 720;
float config::viewDistance = 1000.0f;
bool config::bDevMode = false;

float config::getAspectRatio() { return renderWidth / (float)renderHeight; }
