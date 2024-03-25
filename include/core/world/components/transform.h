#pragma once

#include "component.h"

struct WTransformComponent : public WComponent {
  struct TransformComponentData {
    EComponentType typeId = EComponentType::Transform;

    glm::mat4 transform;

    // Data used in actual transformation calculations
    // Forward vector doubles as a 'look at' target for cameras
    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(0.0f);
    glm::quat orientation = glm::quat(rotation);
    glm::vec3 forwardVector = { 0.0f, 0.0f, 1.0f };

    struct {
      glm::vec3 forwardVector = { 0.0f, 0.0f, 1.0f };
    } initial;

    // was transformation data changed
    bool wasUpdated = false;

    glm::mat4& getModelTransformationMatrix(bool* updateResult = nullptr);
  } data;

  WTransformComponent(ABase* pActor) { typeId = EComponentType::Transform; pOwner = pActor; }

  void showUIElement() override;
};