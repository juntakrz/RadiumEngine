#pragma once

#include "core/objects.h"
#include "core/managers/player.h"
#include "core/managers/world.h"
#include "core/world/components/componentevents.h"

class ACamera;
struct WComponent;

class ABase {
 protected:
  // Names must be set using setName method
  std::string m_name = "";
  std::string m_previousName = "";
  EActorType m_typeId = EActorType::Base;       // TODO: DEPRECATED
  EActorControlMode m_controlMode = EActorControlMode::Spacecraft;
  int32_t m_UID = -1;

  ComponentEventSystem m_eventSystem;

  std::unordered_map<std::type_index, std::unique_ptr<WComponent>> m_pComponents;
  WAttachmentInfo attachmentInfo;

  bool m_isVisible = true;

  glm::vec3 m_forwardVector = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 m_upVector = glm::vec3(0.0f, 1.0f, 0.0f);
  const glm::vec3 m_defaultForwardVector = glm::vec3(0.0f, 0.0f, 1.0f);
  const glm::vec3 m_defaultUpVector = glm::vec3(0.0f, 1.0f, 0.0f);

  core::MPlayer* m_pController = nullptr;

 public:
  ABase() = default;
  ABase(const uint32_t UID);
  virtual ~ABase() {};

  // try to get this actor as its real subclass
  // example: ACamera* camera = actor.getAs<ACamera>();
  template <typename T>
  T* getAs() noexcept {
    return dynamic_cast<T*>(this);
  };

  /* Abstraction methods for transform component, which should always be present for every actor */
  const glm::mat4& getModelTransformationMatrix() noexcept;

  virtual void setTranslation(float x, float y, float z, bool isDelta = false) noexcept;
  virtual void setTranslation(const glm::vec3& newLocation, bool isDelta = false) noexcept;
  virtual void setRotation(float x, float y, float z, bool isInRadians = false, bool isDelta = false) noexcept;
  virtual void setRotation(const glm::vec3& newRotation, bool isInRadians = false, bool isDelta = false) noexcept;
  void setScale(const glm::vec3& newScale, bool isDelta = false) noexcept;
  void setScale(float newScale, bool isDelta = false) noexcept;

  const glm::vec3& getTranslation() noexcept;
  const glm::vec3& getRotation() noexcept;
  const glm::quat& getOrientation() noexcept;
  const glm::vec3& getScale() noexcept;

  virtual void setTranslationModifier(float newModifier);
  virtual void setRotationModifier(float newModifier);
  virtual void setScalingModifier(float newModifier);
  /* End of transform component abstraction methods */

  void setForwardVector(const glm::vec3& newVector);
  const glm::vec3& getForwardVector();
  const glm::vec3& getDefaultForwardVector();

  void setUpVector(const glm::vec3& newVector);
  const glm::vec3& getUpVector();
  const glm::vec3& getDefaultUpVector();

  void setControlMode(EActorControlMode newMode);
  EActorControlMode getControlMode();

  // Should only be used by MPlayer class controller methods
  void onControllerMovement(const glm::vec3& vector, const bool isRotation);
  void onControlled(core::MPlayer* pController);
  void onFreed();

  core::MPlayer* getController();

  void setName(const std::string& name);
  const std::string& getName();
  const std::string& getPreviousName();

  virtual const EActorType& getTypeId();

  const uint32_t getUID();

  void setVisibility(const bool isVisible);
  const bool isVisible();

  void attachTo(ABase* pTarget, EAttachmentMode newMode);
  void detach();

  ComponentEventSystem& getEventSystem() { return m_eventSystem; }

  template<typename T>
  T* getComponent() {
    if (m_pComponents.contains(typeid(T))) return dynamic_cast<T*>(m_pComponents[typeid(T)].get());
    return nullptr;
  }

  template<typename T>
  T* addComponent() {
    if (getComponent<T>()) {
      RE_LOG(Warning, "Failed to add component to '%s', it's already added.", m_name.c_str());
      return dynamic_cast<T*>(m_pComponents[typeid(T)].get());
    }

    m_pComponents[typeid(T)] = std::make_unique<T>(this);
    return dynamic_cast<T*>(m_pComponents[typeid(T)].get());
  }

  void updateComponents();
  void drawComponentUIElements();
};