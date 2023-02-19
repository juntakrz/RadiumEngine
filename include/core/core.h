#pragma once

namespace core {

extern class MGraphics* graphics;

void run();
void mainEventLoop();
void stop(TResult cause);

static void loadCoreConfig(const wchar_t* path = RE_PATH_CONFIG);
}