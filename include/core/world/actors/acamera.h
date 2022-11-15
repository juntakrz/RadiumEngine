#pragma once

#include "core/world/actors/abase.h"

class ACamera : public ABase {
 private:
  float m_deltaMove = 1.0f;
  float m_deltaRotation = 0.03f;

  // camera specific vectors: up and forward directions
  glm::vec3 m_vecUp = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 m_vecFwd = glm::vec3(0.0f, 0.0f, 1.0f);

  // processed vectors
  glm::vec3 m_vecPos, m_vecFocus;

  // view data
  glm::vec4 m_perspData;
  glm::vec4 m_orthoData;

  glm::mat4 m_view;        // view matrix
  glm::mat4 m_projection;  // projection matrix

  // view mode - lookAt object or lookTo direction
  bool m_lookAt = false;
  std::string m_targetId = "";        // if empty - target pos is used
  ACamera* m_pTargetCam = nullptr;    // if nullptr - won't follow

 public:
  ACamera() noexcept {};
  virtual ~ACamera() override {};

  // sets camera projection matrix as perspective
  void setPerspective(float FOV, float aspectRatio, float nearZ,
                        float farZ) noexcept;

  // get view matrix for current camera position and rotation
  glm::mat4& view();

  // get projection matrix using currently selected camera mode
  glm::mat4& projection() { return m_projection; };

  void setUpVector(glm::vec4 upVector);
  void setUpVector(float x, float y, float z);
  /*
  void Move(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void Move(const glm::vec3& delta) noexcept;

  void Rotate(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept;
  void Rotate(const glm::vec3& delta) noexcept;

  void SetAsOrthographic() noexcept;
  void SetAsOrthographic(float width, float height, float nearZ,
                         float farZ) noexcept;

  void SetView() noexcept;      // calculate camera view
  void SetViewProj() noexcept;  // calculate view*proj matrix

  // WIP
  void LookAt(float x, float y, float z) noexcept;
  void LookAt(std::string objectId) noexcept;
  //
  bool LookAtCameraTarget(ACamera* pCam) noexcept;  // nullptr to stop following

  void EnableLookAt() noexcept;
  void DisableLookAt() noexcept;

  const glm::vec3& GetFocus() const noexcept;
  const glm::vec4& GetFocusVector() const noexcept;

  void SetUpVector(float x = 0.0f, float y = 1.0f, float z = 0.0f) noexcept;
  const glm::vec4& GetUpVector() const noexcept;

  void SetMovementDelta(float delta) noexcept;
  const float& GetMovementDelta() const noexcept;

  void SetRotationDelta(float delta) noexcept;
  const float& GetRotationDelta() const noexcept;

  uint8_t TypeId();*/
};