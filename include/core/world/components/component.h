#pragma once

class ABase;

class WComponent {
public:
  virtual void showUIElement(ABase* pActor) { ImGui::Text("Error. Base WComponent is a parent template and should never be used as is."); };
};