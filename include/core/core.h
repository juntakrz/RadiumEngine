#pragma once

namespace core {
void run();
void mainEventLoop();
void stop(TResult cause);

static void loadCoreConfig(const wchar_t* path = RE_PATH_CONFIG);
}