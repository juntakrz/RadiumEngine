#pragma once

#include "core/world/actors/base.h"

class ACamera : public ABase {
 protected:
  EActorType m_typeId = EActorType::Camera;

  struct {
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 focusPoint = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec4 perspectiveData;
    glm::vec4 orthographicData;
    bool bAnchorFocusPoint = false;  // rotate around focus point if true
    ABase* targetActor = nullptr;    // use this actor as a focus point
  } m_viewData;

  glm::mat4 m_view;        // view matrix
  glm::mat4 m_projection;  // projection matrix
  float m_pitch = 0.0f;
  float m_yaw = 0.0f;

 private:
   // uses pitch limit set in config.h
   template<typename T>
  T clampCameraPitch(T pitch) {
    return pitch < -config::pitchLimit  ? -config::pitchLimit
           : pitch > config::pitchLimit ? config::pitchLimit
                                        : pitch;
  }

 public:
  ACamera() noexcept {};
  virtual ~ACamera() override{};

  // sets camera projection matrix as perspective
  void setPerspective(float FOV, float aspectRatio, float nearZ,
                      float farZ) noexcept;
  const glm::vec4& getPerspective() noexcept;

  // get view matrix for current camera position and rotation
  glm::mat4& getView();

  // get projection matrix using currently selected camera mode
  glm::mat4& getProjection();

  void setUpVector(float x, float y, float z) noexcept;
  void setUpVector(const glm::vec3& upVector) noexcept;

  void setAnchorPoint(float x, float y, float z) noexcept;
  void setAnchorPoint(glm::vec3& newAnchorPoint) noexcept;

  virtual void translate(const glm::vec3& delta) noexcept override;

  // set rotation in degrees
  virtual void setRotation(float x, float y, float z) noexcept override;
  virtual void setRotation(const glm::vec3& newRotation) noexcept override;
  virtual void setRotation(const glm::quat& newRotation) noexcept override;

  virtual void rotate(const glm::vec3& vector, float angle) noexcept override;

  virtual void setFOV(float FOV) noexcept;
  virtual void setAspectRatio(float ratio) noexcept;
};