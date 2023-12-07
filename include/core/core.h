#pragma once

namespace core {

// rendering manager, directly communicates with Vulkan API
extern class MRenderer& renderer;

// window manager, uses GLFW to manage windows
extern class MWindow& window;

// input manager, uses GLFW to manage user input
extern class MInput& input;

// script manager, parses data from JSON
extern class MScript& script;

// actors manager, manages interactive entities in the 3D world
extern class MActors& actors;

// animations manager, provides animations to models
extern class MAnimations& animations;

// debug manager, handles debugger integration
extern class MDebug& debug;

// materials manager, manages textures, shaders and materials
extern class MResources& resources;

// player manager, provides interaction between the player and everything else
extern class MPlayer& player;

// reference manager, stores fast references to various kinds of objects
extern class MRef& ref;

// time manager, generates per frame ticks and tracks time
extern class MTime& time;

// world manager, manages models and meshes in a 3D world
extern class MWorld& world;

void run();
void mainEventLoop();
void stop(TResult cause);

TResult create();
void destroy();

void drawFrame();

void loadCoreConfig(const wchar_t* path = RE_PATH_CONFIG);
void loadDevelopmentConfig(const wchar_t* path = RE_PATH_DEVCONFIG);
}