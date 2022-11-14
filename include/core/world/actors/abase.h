#pragma once

/*
 * Base class for manipulated engine objects like models, cameras and lights
 * contains common variables and methods either used as is or overloaded by the
 * inheriting classes
 */

class ABase {
 public:
  std::string m_name;
  glm::mat4 m_matMain = glm::mat4(1.0f);

 protected:
  struct TransformData {
    // data used in actual transformation calculations
    glm::vec4 translation = {0.0f, 0.0f, 0.0f, 1.0f};
    glm::vec4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    glm::vec4 scaling = {1.0f, 1.0f, 1.0f, 1.0f};

    struct {
      // initial data defined by Set* methods
      glm::vec4 translation = {0.0f, 0.0f, 0.0f, 1.0f};
      glm::vec4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};
      glm::vec4 scaling = {0.0f, 0.0f, 0.0f, 1.0f};
    } initial;

  } transform;

 protected:
  ABase(){};
  virtual ~ABase(){};

 public:
 
  // returns matrix with all transformations applied
  virtual glm::mat4& GetTransform() noexcept;

  // data set used for transformation calculations
  virtual TransformData& GetTransformData() noexcept;

  void setPos(float x, float y, float z) noexcept;
  void setPos(const glm::vec4& pos) noexcept;
  glm::vec4& getPos() noexcept;

  void addPos(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void addPos(const glm::vec4& delta) noexcept;

  void setRotation(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void setRotation(const glm::vec4& rotation) noexcept;
  glm::vec4& getRotation() noexcept;

  void addRotation(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void addRotation(const glm::vec4& delta) noexcept;

  void setScale(float x, float y, float z) noexcept;
  void setScale(const glm::vec4& scale) noexcept;
  glm::vec4& getScale() noexcept;

  void addScale(float x = 1.0f, float y = 1.0f, float z = 1.0f) noexcept;
  void addScale(const glm::vec4& delta) noexcept;

  uint8_t TypeId();
};