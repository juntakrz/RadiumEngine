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
    glm::vec3 absoluteForwardVector = { 0.0f, 0.0f, 1.0f };
    
    // x - translation delta, y - rotation delta, z - scale delta
    glm::vec3 deltaModifiers = glm::vec3(1.0f);

    // was transformation data changed
    bool requiresUpdate = false;
  } data;

  WTransformComponent(ABase* pActor);

  const glm::mat4& getModelTransformationMatrix();

  void setTranslation(float x, float y, float z, bool isDelta = false);
  void setTranslation(const glm::vec3& newTranslation, bool isDelta = false);

  void setRotation(float x, float y, float z, bool isInRadians = false, bool isDelta = false);
  void setRotation(const glm::vec3& newRotation, bool isInRadians = false, bool isDelta = false);

  void setScale(float newScale, bool isDelta = false);
  void setScale(float x, float y, float z, bool isDelta = false);
  void setScale(const glm::vec3& newScale, bool isDelta = false);

  void setForwardVector(const glm::vec3& newForwardVector);
  void setAbsoluteForwardVector(const glm::vec3& newForwardVector);

  void setTranslationDeltaModifier(float newModifier);
  void setRotationDeltaModifier(float newModifier);
  void setScaleDeltaModifier(float newModifier);

  const glm::vec3& getTranslation();
  const glm::vec3& getRotation();
  const glm::quat& getOrientation();
  const glm::vec3& getScale();

  const glm::vec3& getForwardVector();
  const glm::vec3& getAbsoluteForwardVector();

  const glm::vec3& getDeltaModifiers();

  void update() override;
  void drawComponentUI() override;
};