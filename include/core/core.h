#pragma once

namespace core {

extern class mrenderer& graphics;

void run();
void mainEventLoop();
void stop(TResult cause);

TResult create();
void destroy();

TResult drawFrame();

static void loadCoreConfig(const wchar_t* path = RE_PATH_CONFIG);
}