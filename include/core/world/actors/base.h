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
  EActorType m_typeId = EActorType::Base;

  struct TransformationData {
    // data used in actual transformation calculations
    glm::vec3 translation = {0.0f, 0.0f, 0.0f};
    glm::quat rotation = {0.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scaling = {1.0f, 1.0f, 1.0f};
    glm::vec3 frontVector = {0.0f, 0.0f, 1.0f};

    struct {
      // initial data defined by Set* methods
      glm::vec3 translation = {0.0f, 0.0f, 0.0f};
      glm::quat rotation = {0.0f, 0.0f, 0.0f, 0.0f};
      glm::vec3 scaling = {1.0f, 1.0f, 1.0f};
    } initial;

  } m_transformationData;

  float m_translationModifier = 1.0f;
  float m_rotationModifier = 1.0f;
  float m_scalingModifier = 1.0f;

  glm::mat4 m_transformationMatrix = glm::mat4(1.0f);

 public:
  std::string name;

 protected:
  ABase(){};
  virtual ~ABase(){};

 public:
  // try to get this actor as its real subclass
  // example: ACamera* camera = actor.getAs<ACamera>();
  template <typename T>
  T* getAs() noexcept {
    return dynamic_cast<T*>(this);
  };

  // returns matrix with all transformations applied
  virtual glm::mat4& getTransformation() noexcept;

  // data set used for transformation calculations
  virtual TransformationData& getTransformData() noexcept;

  virtual void setLocation(const glm::vec3& newLocation) noexcept;
  virtual glm::vec3& getLocation() noexcept;

  virtual void translate(const glm::vec3& delta) noexcept;

  virtual void setRotation(const glm::vec3& newRotation) noexcept;
  virtual void setRotation(const glm::vec3& newVector, float newAngle) noexcept;
  virtual void setRotation(const glm::quat& newRotation) noexcept;
  virtual glm::quat& getRotation() noexcept;
  virtual glm::vec3 getRotationAngles() noexcept;

  virtual void rotate(const glm::vec3& delta) noexcept;
  virtual void rotate(const glm::vec3& vector, float angle) noexcept;
  virtual void rotate(const glm::quat& delta) noexcept;

  virtual void setScale(const glm::vec3& scale) noexcept;
  virtual glm::vec3& getScale() noexcept;

  virtual void scale(const glm::vec3& delta) noexcept;

  void setTranslationModifier(float newModifier);
  void setRotationModifier(float newModifier);
  void setScalingModifier(float newModifier);

  const EActorType& typeId();
};