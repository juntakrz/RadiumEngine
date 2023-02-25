#pragma once

/*
 * Base class for manipulated engine objects like models, cameras and lights
 * contains common variables and methods either used as is or overloaded by the
 * inheriting classes
 */

enum EActorType {
  Base,
  Camera,
  Pawn
};

class ACamera;

class ABase {
 protected:
  EActorType m_typeId = EActorType::Base;

  struct TransformData {
    // data used in actual transformation calculations
    glm::vec3 translation = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 scaling = {1.0f, 1.0f, 1.0f};
    glm::vec3 frontVector = {0.0f, 0.0f, 1.0f};

    struct {
      // initial data defined by Set* methods
      glm::vec3 translation = {0.0f, 0.0f, 0.0f};
      glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
      glm::vec3 scaling = {1.0f, 1.0f, 1.0f};
    } initial;

  } m_transformData;

  float m_translationSpeed = 1.0f;
  float m_rotationSpeed = 1.0f;
  float m_scalingSpeed = 1.0f;

  glm::mat4 m_mainTransformationMatrix = glm::mat4(1.0f);

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
  virtual glm::mat4& getTransform() noexcept;

  // data set used for transformation calculations
  virtual TransformData& getTransformData() noexcept;

  virtual void setPos(float x, float y, float z) noexcept;
  virtual void setPos(const glm::vec3& pos) noexcept;
  virtual glm::vec3& getPos() noexcept;

  virtual void translate(float x, float y, float z) noexcept;
  virtual void translate(const glm::vec3& delta) noexcept;

  virtual void setRotation(float x, float y, float z) noexcept;
  virtual void setRotation(const glm::vec3& rotation) noexcept;
  virtual glm::vec3& getRotation() noexcept;

  virtual void rotate(float x, float y, float z) noexcept;
  virtual void rotate(const glm::vec3& delta) noexcept;

  virtual void setScale(float x, float y, float z) noexcept;
  virtual void setScale(const glm::vec3& scale) noexcept;
  virtual glm::vec3& getScale() noexcept;

  virtual void scale(float x, float y, float z) noexcept;
  virtual void scale(const glm::vec3& delta) noexcept;

  void setTranslationSpeed(const float& newSpeed);
  void setRotationSpeed(const float& newSpeed);
  void setScalingSpeed(const float& newSpeed);

  const uint32_t typeId();
};