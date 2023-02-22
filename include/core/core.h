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

void loadCoreConfig(const wchar_t* path = RE_PATH_CONFIG);
void loadDevelopmentConfig(const wchar_t* path = RE_PATH_DEVCONFIG);
void compileShaders_Debug();
}