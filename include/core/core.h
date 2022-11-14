#pragma once

// engine core managers
extern class MGraphics* mgrGfx;
extern class MWindow* mgrWnd;
extern class MDebug* mgrDbg;
extern class MInput* mgrInput;
extern class MModel* mgrModel;
extern class MScript* mgrScript;
extern class MRef* mgrRef;

namespace core {
void run();
void mainEventLoop();
void stop(TResult cause);

static void loadCoreConfig(const wchar_t* path = RE_PATH_CONFIG);
}