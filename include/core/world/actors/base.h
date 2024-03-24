#pragma once

#include "core/objects.h"
#include "core/managers/world.h"

/*
 * Base class for manipulated engine objects like models, cameras and lights
 * contains common variables and methods either used as is or overloaded by the
 * inheriting classes
 */

class ACamera;
class WComponent;

class ABase {
 protected:
  // Names must be set using setName method
  std::string m_name = "";
  std::string m_previousName = "";
  EActorType m_typeId = EActorType::Base;
  int32_t m_UID = -1;

  struct TransformationData {
    // data used in actual transformation calculations
    // forward vector doubles as a 'look at' target
    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scaling = glm::vec3(0.0f);
    glm::quat orientation = glm::quat(rotation);
    glm::vec3 forwardVector = {0.0f, 0.0f, 1.0f};

    struct {
      glm::vec3 forwardVector = {0.0f, 0.0f, 1.0f};
    } initial;

    // was transformation data changed
    bool wasUpdated = false;
  } m_transformationData;

  std::unordered_map<std::type_index, WComponent*> m_pComponents;
  std::vector<WAttachmentInfo> m_pAttachments;

  glm::mat4 m_transformationMatrix = glm::mat4(1.0f);

  float m_translationModifier = 1.0f;
  float m_rotationModifier = 1.0f;
  float m_scalingModifier = 1.0f;
  bool m_isVisible = true;

 protected:
  // SIMD stuff, no error checks
  void copyVec3ToMatrix(const float* vec3, float* matrixColumn) noexcept;

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

  // returns root matrix with all transformations applied
  virtual glm::mat4& getRootTransformationMatrix() noexcept;

  // data set used for transformation calculations
  virtual TransformationData& getTransformData() noexcept;

  virtual void setTranslation(float x, float y, float z) noexcept;
  virtual void setTranslation(const glm::vec3& newLocation) noexcept;
  virtual glm::vec3& getTranslation() noexcept;

  virtual void translate(const glm::vec3& delta) noexcept;

  // set absolute rotation in degrees
  virtual void setRotation(float x, float y, float z) noexcept {};

  // set absolute rotation in radians
  virtual void setRotation(const glm::vec3& newRotation, const bool inRadians = false) noexcept;
  const glm::vec3& getRotation() noexcept;
  const glm::quat& getOrientation() noexcept;

  virtual void rotate(const glm::vec3& delta, const bool ignoreFrameTime = false) noexcept;
  virtual void rotate(const glm::vec3& vector, float angle) noexcept {};

  void setScale(const glm::vec3& scale) noexcept;
  void setScale(float scale) noexcept;
  const glm::vec3& getScale() noexcept;

  virtual void scale(const glm::vec3& delta) noexcept;

  virtual void setTranslationModifier(float newModifier);
  virtual void setRotationModifier(float newModifier);
  virtual void setScalingModifier(float newModifier);

  virtual void setForwardVector(const glm::vec3& newVector);
  virtual glm::vec3& getForwardVector();

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
  bool addComponent() {
    if (m_pComponents.contains(typeid(T))) {
      RE_LOG(Error, "Failed to add component to '%s', it's already added.", m_name.c_str());
      return false;
    }

    T* pComponent = core::MWorld::get().getComponent<T>();
    if (!pComponent) {
      RE_LOG(Error, "Failed to add component to '%s', no template for this type exists.", m_name.c_str());
      return false;
    }

    m_pComponents[typeid(T)] = pComponent;
    return true;
  }

  void drawComponentUIElements();
};