#pragma once

#include "core/objects.h"
#include "core/managers/world.h"

/*
 * Base class for manipulated engine objects like models, cameras and lights
 * contains common variables and methods either used as is or overloaded by the
 * inheriting classes
 */

class ACamera;
struct WComponent;

class ABase {
 protected:
  // Names must be set using setName method
  std::string m_name = "";
  std::string m_previousName = "";
  EActorType m_typeId = EActorType::Base;
  int32_t m_UID = -1;

  std::unordered_map<std::type_index, std::unique_ptr<WComponent>> m_pComponents;
  std::vector<WAttachmentInfo> m_pAttachments;

  bool m_isVisible = true;

 protected:
  virtual void updateAttachments();

 public:
  ABase() = default;
  ABase(const uint32_t UID) { m_UID = UID; };
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
  void setForwardVector(const glm::vec3& newVector);
  void setAbsoluteForwardVector(const glm::vec3& newVector);

  const glm::vec3& getTranslation() noexcept;
  const glm::vec3& getRotation() noexcept;
  const glm::quat& getOrientation() noexcept;
  const glm::vec3& getScale() noexcept;
  const glm::vec3& getForwardVector();
  const glm::vec3& getAbsoluteForwardVector();

  virtual void setTranslationModifier(float newModifier);
  virtual void setRotationModifier(float newModifier);
  virtual void setScalingModifier(float newModifier);
  /* End of transform component abstraction methods */

  void setName(const std::string& name);
  const std::string& getName();
  const std::string& getPreviousName();

  virtual const EActorType& getTypeId();

  const uint32_t getUID();

  void setVisibility(const bool isVisible);
  const bool isVisible();

  virtual void attachTo(ABase* pTarget, const bool toTranslation,
                        const bool toRotation, const bool toForwardVector);

  // Were any of the actor transformations updated (clear update status if required)
  virtual bool wasUpdated(const bool clearStatus = false);

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

  void drawComponentUIElements();
};