#pragma once

#include "pch.h"

class ABase;
struct WComponent;

struct ComponentEvent {
  ABase* pEventOwner = nullptr;

  virtual ~ComponentEvent() {}
};

struct TransformUpdateComponentEvent : public ComponentEvent {
  glm::vec3 translation;
};

struct ControllerTranslationComponentEvent : public ComponentEvent {
  glm::vec3 controllerTranslationDelta;
};

struct ControllerRotationComponentEvent : public ComponentEvent {
  glm::vec3 controllerRotationDelta;
};

class ComponentEventSystem {
  using ComponentDelegate = std::function<void(const ComponentEvent&)>;

private:
  // Map: [event type] - vector of [WComponent]
  std::unordered_map<std::type_index, std::vector<ComponentDelegate>> m_delegates;

public:
  template<typename EventType, typename ClassType>
  void addDelegate(ClassType* instance, void(ClassType::*function)(const ComponentEvent&)) {
    if (!function || !instance) {
      RE_LOG(Error, "Couldn't add component event delegate, nullptr was received.");
    }

    m_delegates[typeid(EventType)].emplace_back(std::bind(function, instance, std::placeholders::_1));
  }

  template<typename EventType>
  void sendEvent(const EventType& newEvent) {
    std::type_index eventTypeId = typeid(EventType);

    if (m_delegates.contains(eventTypeId)) {
      for (auto& func : m_delegates[eventTypeId]) {
        func(newEvent);
      }
    }
  }
};