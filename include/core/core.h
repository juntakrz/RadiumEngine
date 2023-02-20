#pragma once

namespace core {

extern class MRenderer& renderer;
extern class MWindow& window;
extern class MInput& input;
extern class MScript& script;
extern class MActors& actors;
extern class MRef& ref;
extern class MDebug& debug;
extern class MTime& time;

void run();
void mainEventLoop();
void stop(TResult cause);

TResult create();
void destroy();

TResult drawFrame();

static void loadCoreConfig(const wchar_t* path = RE_PATH_CONFIG);
}