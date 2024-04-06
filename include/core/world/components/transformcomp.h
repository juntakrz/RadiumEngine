#pragma once

#include "component.h"

struct WTransformComponent : public WComponent {
  struct {
    EComponentType typeId = EComponentType::Transform;
    EActorControlMode controlMode = EActorControlMode::Spacecraft;

    glm::mat4 transform;

    // Data used in actual transformation calculations
    // Forward vector doubles as a 'look at' target for cameras
    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::quat orientation = glm::quat(rotation);
    
    // x - translation delta, y - rotation delta, z - scale delta
    glm::vec3 deltaModifiers = glm::vec3(1.0f);

    // Origin attachment vector from which rotated attachment vector is calculated
    glm::vec3 baseAttachmentVector = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 attachmentTranslation = glm::vec3(0.0f);

    // Vector to attachment target
    glm::vec3 attachmentVector = glm::vec3(0.0f);

    // Rotation data for attachment vector
    glm::quat attachmentOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    // was transformation data changed
    bool transformRequiresUpdate = false;
  } data;

  WTransformComponent(WActor* pActor);

  glm::mat4& getModelTransformationMatrix();

  void setTranslation(float x, float y, float z, bool isDelta);
  void setTranslation(const glm::vec3& newTranslation, bool isDelta);
  
  void setWorldTranslation(const glm::vec3& newTranslation, bool isDelta);

  void setRotation(float x, float y, float z, bool isInRadians, bool isDelta);
  void setRotation(const glm::vec3& newRotation, bool isInRadians, bool isDelta);

  void setOrientation(const glm::quat& newQuat, const bool isDelta);

  void setScale(float newScale, bool isDelta);
  void setScale(float x, float y, float z, bool isDelta);
  void setScale(const glm::vec3& newScale, bool isDelta);

  void setTranslationDeltaModifier(float newModifier);
  void setRotationDeltaModifier(float newModifier);
  void setScaleDeltaModifier(float newModifier);

  const glm::vec3& getTranslation();
  const glm::vec3& getRotation();
  const glm::quat& getOrientation();
  const glm::vec3& getScale();

  const glm::vec3& getDeltaModifiers();

  void setAttachmentVectorRotation(const glm::vec3& newRotation, const bool isInRadians, const bool isDelta);
  void setAttachmentVectorLength(const float newLength);
  const glm::vec3& getAttachmentVector();

  void forceUpdateTransform();

  void onAttachmentModeChanged(WActor* pNewTarget, EAttachmentMode newMode) override;
  void onOwnerControlled() override;
  void onOwnerFreed() override;
  void onOwnerUpdated() override;

  void update() override;
  void drawComponentUI(const uint32_t index) override;

  // Event delegates
  void handleControllerTranslation(const ComponentEvent& newEvent);
  void handleControllerRotation(const ComponentEvent& newEvent);
  void handleAttachmentTargetTransformUpdated(const ComponentEvent& newEvent);
  void handleAttachmentTargetDestroyed(const ComponentEvent& newEvent);
};