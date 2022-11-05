#pragma once

// engine core managers
extern class MGraphics* mgrGfx;
extern class MWindow* mgrWnd;
extern class MDebug* mgrDbg;

namespace core {
void run();
void mainEventLoop();
void stop(TResult cause);

static void loadCoreConfig(const wchar_t* path = RE_PATH_CONFIG);
}