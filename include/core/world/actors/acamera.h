#pragma once

#include "core/world/actors/abase.h"

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

  // processed vectors
  glm::vec3 m_vecPos, m_vecFocus;

  float m_deltaMove = 1.0f;
  float m_deltaRotation = 0.03f;

  glm::mat4 m_view;        // view matrix
  glm::mat4 m_projection;  // projection matrix

  // view mode - lookAt object or lookTo direction

 public:
  ACamera() noexcept {};
  virtual ~ACamera() override{};

  // sets camera projection matrix as perspective
  void setPerspective(float FOV, float aspectRatio, float nearZ,
                      float farZ) noexcept;

  // get view matrix for current camera position and rotation
  glm::mat4& getView();

  // get projection matrix using currently selected camera mode
  glm::mat4& getProjection();

  void setUpVector(const glm::vec3& upVector);
  void setUpVector(float x, float y, float z);
};