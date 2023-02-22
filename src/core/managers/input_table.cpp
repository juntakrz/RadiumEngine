#include "pch.h"
#include "core/managers/input.h"

void core::MInput::setDefaultInputs() {
  RE_LOG(Log, "Setting up default input bindings and data.");

  // default input bindings
  m_inputBinds["moveForward"] = GLFW_KEY_W;
  m_inputBinds["moveBack"] = GLFW_KEY_S;
  m_inputBinds["moveLeft"] = GLFW_KEY_A;
  m_inputBinds["moveRight"] = GLFW_KEY_D;
  m_inputBinds["moveUp"] = GLFW_KEY_LEFT_SHIFT;
  m_inputBinds["moveDown"] = GLFW_KEY_LEFT_CONTROL;
  m_inputBinds["mouseMode"] = GLFW_KEY_SPACE;
  m_inputBinds["devKey"] = GLFW_KEY_GRAVE_ACCENT;

  // key aliases
  m_inputAliases["Esc"] = GLFW_KEY_ESCAPE;
  m_inputAliases["F1"] = GLFW_KEY_F1;
  m_inputAliases["F2"] = GLFW_KEY_F2;
  m_inputAliases["F3"] = GLFW_KEY_F3;
  m_inputAliases["F4"] = GLFW_KEY_F4;
  m_inputAliases["F5"] = GLFW_KEY_F5;
  m_inputAliases["F6"] = GLFW_KEY_F6;
  m_inputAliases["F7"] = GLFW_KEY_F7;
  m_inputAliases["F8"] = GLFW_KEY_F8;
  m_inputAliases["F9"] = GLFW_KEY_F9;
  m_inputAliases["F10"] = GLFW_KEY_F10;
  m_inputAliases["F11"] = GLFW_KEY_F11;
  m_inputAliases["F12"] = GLFW_KEY_F12;

  m_inputAliases["`"] = GLFW_KEY_GRAVE_ACCENT;
  m_inputAliases["1"] = GLFW_KEY_1;
  m_inputAliases["2"] = GLFW_KEY_2;
  m_inputAliases["3"] = GLFW_KEY_3;
  m_inputAliases["4"] = GLFW_KEY_4;
  m_inputAliases["5"] = GLFW_KEY_5;
  m_inputAliases["6"] = GLFW_KEY_6;
  m_inputAliases["7"] = GLFW_KEY_7;
  m_inputAliases["8"] = GLFW_KEY_8;
  m_inputAliases["9"] = GLFW_KEY_9;
  m_inputAliases["0"] = GLFW_KEY_0;
  m_inputAliases["-"] = GLFW_KEY_MINUS;
  m_inputAliases["="] = GLFW_KEY_EQUAL;

  m_inputAliases["Q"] = GLFW_KEY_Q;
  m_inputAliases["W"] = GLFW_KEY_W;
  m_inputAliases["E"] = GLFW_KEY_E;
  m_inputAliases["R"] = GLFW_KEY_R;
  m_inputAliases["T"] = GLFW_KEY_T;
  m_inputAliases["Y"] = GLFW_KEY_Y;
  m_inputAliases["U"] = GLFW_KEY_U;
  m_inputAliases["I"] = GLFW_KEY_I;
  m_inputAliases["O"] = GLFW_KEY_O;
  m_inputAliases["P"] = GLFW_KEY_P;
  m_inputAliases["["] = GLFW_KEY_LEFT_BRACKET;
  m_inputAliases["]"] = GLFW_KEY_RIGHT_BRACKET;

  m_inputAliases["A"] = GLFW_KEY_A;
  m_inputAliases["S"] = GLFW_KEY_S;
  m_inputAliases["D"] = GLFW_KEY_D;
  m_inputAliases["E"] = GLFW_KEY_F;
  m_inputAliases["F"] = GLFW_KEY_G;
  m_inputAliases["G"] = GLFW_KEY_H;
  m_inputAliases["H"] = GLFW_KEY_J;
  m_inputAliases["J"] = GLFW_KEY_K;
  m_inputAliases["K"] = GLFW_KEY_L;
  m_inputAliases["L"] = GLFW_KEY_M;
  m_inputAliases[";"] = GLFW_KEY_SEMICOLON;
  m_inputAliases["'"] = GLFW_KEY_APOSTROPHE;
  m_inputAliases["\\"] = GLFW_KEY_BACKSLASH;

  m_inputAliases["Z"] = GLFW_KEY_Z;
  m_inputAliases["X"] = GLFW_KEY_X;
  m_inputAliases["C"] = GLFW_KEY_C;
  m_inputAliases["V"] = GLFW_KEY_V;
  m_inputAliases["B"] = GLFW_KEY_B;
  m_inputAliases["N"] = GLFW_KEY_N;
  m_inputAliases["M"] = GLFW_KEY_M;
  m_inputAliases[","] = GLFW_KEY_COMMA;
  m_inputAliases["."] = GLFW_KEY_PERIOD;
  m_inputAliases["/"] = GLFW_KEY_SLASH;

  m_inputAliases["Tab"] = GLFW_KEY_TAB;
  m_inputAliases["CapsLock"] = GLFW_KEY_CAPS_LOCK;
  m_inputAliases["LShift"] = GLFW_KEY_LEFT_SHIFT;
  m_inputAliases["LCtrl"] = GLFW_KEY_LEFT_CONTROL;
  m_inputAliases["Space"] = GLFW_KEY_SPACE;
  m_inputAliases["Backspace"] = GLFW_KEY_BACKSPACE;
  m_inputAliases["Enter"] = GLFW_KEY_ENTER;
  m_inputAliases["RShift"] = GLFW_KEY_RIGHT_SHIFT;
  m_inputAliases["RCtrl"] = GLFW_KEY_RIGHT_CONTROL;
  m_inputAliases["PrintScreen"] = GLFW_KEY_PRINT_SCREEN;
  m_inputAliases["ScrollLock"] = GLFW_KEY_SCROLL_LOCK;

  m_inputAliases["Insert"] = GLFW_KEY_INSERT;
  m_inputAliases["Delete"] = GLFW_KEY_DELETE;
  m_inputAliases["Home"] = GLFW_KEY_HOME;
  m_inputAliases["End"] = GLFW_KEY_END;
  m_inputAliases["PageUp"] = GLFW_KEY_PAGE_UP;
  m_inputAliases["PageDown"] = GLFW_KEY_PAGE_DOWN;
  m_inputAliases["Up"] = GLFW_KEY_UP;
  m_inputAliases["Left"] = GLFW_KEY_LEFT;
  m_inputAliases["Down"] = GLFW_KEY_DOWN;
  m_inputAliases["Right"] = GLFW_KEY_RIGHT;

  m_inputAliases["Num1"] = GLFW_KEY_KP_1;
  m_inputAliases["Num2"] = GLFW_KEY_KP_2;
  m_inputAliases["Num3"] = GLFW_KEY_KP_3;
  m_inputAliases["Num4"] = GLFW_KEY_KP_4;
  m_inputAliases["Num5"] = GLFW_KEY_KP_5;
  m_inputAliases["Num6"] = GLFW_KEY_KP_6;
  m_inputAliases["Num7"] = GLFW_KEY_KP_7;
  m_inputAliases["Num8"] = GLFW_KEY_KP_8;
  m_inputAliases["Num9"] = GLFW_KEY_KP_9;
  m_inputAliases["Num0"] = GLFW_KEY_KP_0;

  m_inputAliases["NumAdd"] = GLFW_KEY_KP_ADD;
  m_inputAliases["NumDecimal"] = GLFW_KEY_KP_DECIMAL;
  m_inputAliases["NumDivide"] = GLFW_KEY_KP_DIVIDE;
  m_inputAliases["NumEnter"] = GLFW_KEY_KP_ENTER;
  m_inputAliases["NumEqual"] = GLFW_KEY_KP_EQUAL;
  m_inputAliases["NumMultiply"] = GLFW_KEY_KP_MULTIPLY;
  m_inputAliases["NumSubtract"] = GLFW_KEY_KP_SUBTRACT;
}