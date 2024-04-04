#pragma once

#include "core/objects.h"
#include "core/managers/player.h"
#include "core/managers/world.h"
#include "core/world/components/components.h"
#include "core/world/components/componentevents.h"

class ACamera;
struct WComponent;

class WActor {
 protected:
  // Names must be set using setName method
  std::string m_name = "";
  std::string m_previousName = "";
  EActorType m_typeId = EActorType::Base;       // TODO: DEPRECATED
  EActorControlMode m_controlMode = EActorControlMode::Spacecraft;
  int32_t m_UID = -1;

  ComponentEventSystem m_eventSystem;

  std::unordered_map<std::type_index, std::vector<std::unique_ptr<WComponent>>> m_pComponents;
  WAttachmentInfo attachmentInfo;

  bool m_isVisible = true;

  glm::vec3 m_forwardVector = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 m_upVector = glm::vec3(0.0f, 1.0f, 0.0f);
  const glm::vec3 m_defaultForwardVector = glm::vec3(0.0f, 0.0f, 1.0f);
  const glm::vec3 m_defaultUpVector = glm::vec3(0.0f, 1.0f, 0.0f);

  core::MPlayer* m_pController = nullptr;

 public:
  WActor() = default;
  WActor(const uint32_t UID);
  virtual ~WActor() {};

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

  void attachTo(WActor* pTarget, EAttachmentMode newMode);
  void detach();

  ComponentEventSystem& getEventSystem() { return m_eventSystem; }

  template<typename T>
  T* getComponent(const size_t index = 0) {
    if (m_pComponents.contains(typeid(T))) {
      if (index >= m_pComponents[typeid(T)].size()) {
        RE_LOG(Error, "Index %d is out of bounds when trying to retrieve component for '%s'."
          "Maximum number of components of this type is %d.",
          index, getName().c_str(), m_pComponents[typeid(T)].size());
        return nullptr;
      }

      return dynamic_cast<T*>(m_pComponents[typeid(T)][index].get());
    }
    return nullptr;
  }

  template<typename T>
  size_t getComponentCount() {
    if (m_pComponents.contains(typeid(T))) {
      return m_pComponents[typeid(T)].size();
    }

    return 0;
  }

  template<typename T>
  T* addComponent() {
    if (getComponent<T>() && (typeid(T) == typeid(WTransformComponent) || typeid(T) == typeid(WCameraComponent))) {
      RE_LOG(Warning, "Failed to add component to '%s', only one component of requested type is allowed per actor.",
        m_name.c_str());
      return dynamic_cast<T*>(m_pComponents[typeid(T)][0].get());
    }

    m_pComponents[typeid(T)].emplace_back(std::move(std::make_unique<T>(this)));
    return dynamic_cast<T*>(m_pComponents[typeid(T)].back().get());
  }

  template<typename T>
  bool removeComponent(T* pComponent) {
    if (!pComponent) {
      RE_LOG(Error, "Failed to remove a component of '%s', nullptr was received.", getName().c_str());
      return false;
    }

    const std::type_index typeIndex = typeid(T);
    if (!m_pComponents.contains(typeIndex)) {
      RE_LOG(Error, "Failed to remove a component of '%s', it was not found.", getName().c_str());
      return false;
    }

    uint32_t componentIndex = 0;
    for (auto& it : m_pComponents[typeIndex]) {
      if (it.get() == pComponent) {
        m_pComponents[typeIndex].erase(m_pComponents[typeIndex].begin() + componentIndex);
        return true;
      }

      ++componentIndex;
    }

    return false;
  }

  void updateComponents();
  void forceUpdateTransform();
  void drawComponentUIElements();
};