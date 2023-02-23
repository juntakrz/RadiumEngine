#pragma once

/*
 * Base class for manipulated engine objects like models, cameras and lights
 * contains common variables and methods either used as is or overloaded by the
 * inheriting classes
 */

enum EActorType {
  Base,
  Camera
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

    struct {
      // initial data defined by Set* methods
      glm::vec3 translation = {0.0f, 0.0f, 0.0f};
      glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
      glm::vec3 scaling = {1.0f, 1.0f, 1.0f};
    } initial;

  } m_transformData;

  glm::mat4 m_mainTransformationMatrix = glm::mat4(1.0f);

 public:
  std::string name;
  
  float translationSpeed = 1.0f;
  float rotationSpeed = 1.0f;

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

  void setPos(float x, float y, float z) noexcept;
  void setPos(const glm::vec3& pos) noexcept;
  glm::vec3& getPos() noexcept;

  void translate(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void translate(const glm::vec3& delta) noexcept;

  void setRotation(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void setRotation(const glm::vec3& rotation) noexcept;
  glm::vec3& getRotation() noexcept;

  void rotate(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void rotate(const glm::vec3& delta) noexcept;

  void setScale(float x, float y, float z) noexcept;
  void setScale(const glm::vec3& scale) noexcept;
  glm::vec3& getScale() noexcept;

  void scale(float x = 1.0f, float y = 1.0f, float z = 1.0f) noexcept;
  void scale(const glm::vec3& delta) noexcept;

  const uint32_t typeId();
};