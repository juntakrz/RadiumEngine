#pragma once

// engine core managers
extern class MGraphics* mgrGfx;
extern class MWindow* mgrWnd;
extern class MDebug* mgrDbg;

namespace core {
void run();
void mainEventLoop();
void stop(TResult cause);
}