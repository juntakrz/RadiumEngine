#pragma once

#include "core/world/actors/abase.h"

class ACamera : public ABase {
 private:
  float m_deltaMove = 1.0f;
  float m_deltaRotation = 0.03f;

  // camera specific vectors: up and forward directions
  glm::vec4 m_vecUp = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
  glm::vec4 m_vecFwd = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

  // processed vectors
  glm::vec4 m_vecPos, m_vecFocus;

  // view data
  glm::vec4 m_perspData;
  glm::vec4 m_orthoData;

  // view mode - lookAt object or lookTo direction
  bool m_lookAt = false;
  std::string m_targetId = "";        // if empty - target pos is used
  ACamera* m_pTargetCam = nullptr;    // if nullptr - won't follow

 public:
  RMVPMatrices m_ModelViewProj;

 public:
  ACamera() noexcept {};
  virtual ~ACamera() override {};

  // get reference to model - view - projection matrices
  RMVPMatrices& getMVP() { return m_ModelViewProj; };

  void setPerspective(float FOV, float aspectRatio, float nearZ,
                        float farZ) noexcept;

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