#pragma once

#include "core/world/actors/base.h"

class ACamera : public ABase {
 protected:
  EActorType m_typeId = EActorType::Camera;
  ECameraProjection m_projectionType = ECameraProjection::Perspective;

  struct {
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec4 perspectiveData;
    glm::vec4 orthographicData;
    bool anchorFocusPoint = false;
    bool ignorePitchLimit = false;
  } m_viewData;

  struct {
    bool followTarget = false;      // camera will translate together with its target
    bool useForwardVector = false;  // look at forward vector of the target
    ABase* pTarget = nullptr;
  } m_target;

  glm::mat4 m_view;        // view matrix
  glm::mat4 m_projection;  // projection matrix
  float m_pitch = 0.0f;
  float m_yaw = 0.0f;

  uint32_t m_ViewBufferIndex = -1;

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

  void setOrthographic(float horizontal, float vertical, float nearZ,
                       float farZ) noexcept;

  // get view matrix for current camera position and rotation
  glm::mat4& getView();

  // get projection matrix using currently selected camera mode
  glm::mat4& getProjection();

  ECameraProjection getProjectionType();

  void setUpVector(float x, float y, float z) noexcept;
  void setUpVector(const glm::vec3& upVector) noexcept;

  void setLookAtTarget(ABase* pTarget, const bool useForwardVector,
                       const bool attach) noexcept;

  virtual void translate(const glm::vec3& delta) noexcept override;

  // set rotation in degrees
  virtual void setRotation(float x, float y, float z) noexcept override;
  virtual void setRotation(const glm::vec3& newRotation) noexcept override;
  virtual void setRotation(const glm::quat& newRotation) noexcept override;

  virtual void rotate(const glm::vec3& vector, float angle) noexcept override;

  virtual void setIgnorePitchLimit(const bool newValue);

  virtual void setFOV(float FOV) noexcept;
  virtual void setAspectRatio(float ratio) noexcept;

  virtual void setViewBufferIndex(const uint32_t newIndex);
  virtual const uint32_t getViewBufferIndex();
};