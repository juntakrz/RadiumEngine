#pragma once

/*
 * Base class for manipulated engine objects like models, cameras and lights
 * contains common variables and methods either used as is or overloaded by the
 * inheriting classes
 */

class ABase {
 public:
  std::string name;
  glm::mat4 mxMain = glm::mat4(1.0f);

 protected:
  struct TransformData {
    // data used in actual transformation calculations
    glm::vec3 translation = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 scaling = {1.0f, 1.0f, 1.0f};

    struct {
      // initial data defined by Set* methods
      glm::vec3 translation = {0.0f, 0.0f, 0.0f};
      glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
      glm::vec3 scaling = {0.0f, 0.0f, 0.0f};
    } initial;

  } transform;

 protected:
  ABase(){};
  virtual ~ABase(){};

 public:
 
  /*
  // returns matrix with all transformations applied
  virtual glm::mat4& GetTransform() noexcept;

  // data set used for transformation calculations
  virtual TransformData& GetTransformData() noexcept;

  void SetPos(float x, float y, float z) noexcept;
  void SetPos(const glm::vec3& pos) noexcept;
  glm::vec3& GetPos() noexcept;

  void Move(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void Move(const glm::vec3& delta) noexcept;

  void SetRotation(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void SetRotation(const glm::vec3& rotation) noexcept;
  glm::vec3& GetRotation() noexcept;

  void Rotate(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void Rotate(const glm::vec3& delta) noexcept;

  void SetScale(float x, float y, float z) noexcept;
  void SetScale(const glm::vec3& scale) noexcept;
  glm::vec3& GetScale() noexcept;

  void Scale(float x = 1.0f, float y = 1.0f, float z = 1.0f) noexcept;
  void Scale(const glm::vec3& delta) noexcept;

  uint8_t TypeId();*/
};