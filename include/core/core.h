#pragma once

namespace core {
// Actors manager, manages interactive entities in the 3D world
extern class MActors& actors;

// Animations manager, provides animations to models
extern class MAnimations& animations;

// Debug manager, handles debugger integration
extern class MDebug& debug;

// Graphical user interface manager
extern class MGUI& gui;

// Input manager, uses GLFW to manage user input
extern class MInput& input;

// Player manager, provides interaction between the player and everything else
extern class MPlayer& player;

// Reference manager, stores fast references to various kinds of objects
extern class MRef& ref;

// Rendering manager, directly communicates with Vulkan API
extern class MRenderer& renderer;

// Materials manager, manages textures, shaders and materials
extern class MResources& resources;

// Script manager, parses data from JSON
extern class MScript& script;

// Time manager, generates per frame ticks and tracks time
extern class MTime& time;

// Window manager, uses GLFW to manage windows
extern class MWindow& window;

// World manager, manages models and meshes in a 3D world
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