#pragma once

#include "core/objects.h"

/*
 * Base class for manipulated engine objects like models, cameras and lights
 * contains common variables and methods either used as is or overloaded by the
 * inheriting classes
 */

class ACamera;

class ABase {
 protected:
  std::string m_name = "$NONAME$";
  EActorType m_typeId = EActorType::Base;

  struct TransformationData {
    // data used in actual transformation calculations
    // forward vector doubles as a 'look at' target
    glm::vec3 translation = {0.0f, 0.0f, 0.0f};
    glm::quat rotation = {0.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scaling = {1.0f, 1.0f, 1.0f};
    glm::vec3 forwardVector = {0.0f, 0.0f, 1.0f};

    struct {
      // initial data defined by Set* methods
      glm::vec3 translation = {0.0f, 0.0f, 0.0f};
      glm::quat rotation = {0.0f, 0.0f, 0.0f, 1.0f};
      glm::vec3 scaling = {1.0f, 1.0f, 1.0f};
      glm::vec3 forwardVector = {0.0f, 0.0f, 1.0f};
    } initial;

    // was transformation data changed
    bool wasUpdated = false;
  } m_transformationData;

  std::vector<WAttachmentInfo> m_pAttachments;

  glm::mat4 m_transformationMatrix = glm::mat4(1.0f);

  float m_translationModifier = 1.0f;
  float m_rotationModifier = 1.0f;
  float m_scalingModifier = 1.0f;
  bool m_isVisible = true;

 protected:
  ABase(){};
  virtual ~ABase(){};

  // SIMD stuff, no error checks
  void copyVec3ToMatrix(const float* vec3, float* matrixColumn) noexcept;

  virtual void updateAttachments();

 public:
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

  virtual void setLocation(float x, float y, float z) noexcept;
  virtual void setLocation(const glm::vec3& newLocation) noexcept;
  virtual glm::vec3& getLocation() noexcept;

  virtual void translate(const glm::vec3& delta) noexcept;

  // set absolute rotation in degrees
  virtual void setRotation(float x, float y, float z) noexcept {};

  // set absolute rotation in radians
  virtual void setRotation(const glm::vec3& newRotation) noexcept;
  virtual void setRotation(const glm::vec3& newVector, float newAngle) noexcept;
  virtual void setRotation(const glm::quat& newRotation) noexcept;
  virtual glm::quat& getRotation() noexcept;
  virtual glm::vec3 getRotationAngles() noexcept;

  virtual void rotate(const glm::vec3& delta) noexcept;
  virtual void rotate(const glm::vec3& vector, float angle) noexcept;
  virtual void rotate(const glm::quat& delta) noexcept;

  virtual void setScale(const glm::vec3& scale) noexcept;
  virtual void setScale(float scale) noexcept;
  virtual glm::vec3& getScale() noexcept;

  virtual void scale(const glm::vec3& delta) noexcept;

  virtual void setTranslationModifier(float newModifier);
  virtual void setRotationModifier(float newModifier);
  virtual void setScalingModifier(float newModifier);

  virtual void setForwardVector(const glm::vec3& newVector);
  virtual glm::vec3& getForwardVector();

  virtual void setName(const char* name);
  virtual const std::string& getName();

  virtual const EActorType& getTypeId();

  virtual void setVisibility(const bool isVisible);
  virtual const bool isVisible();

  virtual void attachTo(ABase* pTarget, const bool toTranslation,
                        const bool toRotation, const bool toForwardVector);

  // Were any of the actor transformations updated (clear update status if required)
  virtual bool wasUpdated(const bool clearStatus = false);
};