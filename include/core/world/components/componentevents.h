#pragma once

#include "pch.h"

class WActor;
struct WComponent;

struct ComponentEvent {
  WActor* pEventOwner = nullptr;

  virtual ~ComponentEvent() {}
};

struct TransformUpdateComponentEvent : public ComponentEvent {
  glm::vec3 translation;
  glm::vec3 rotation;
  glm::quat orientation;
  glm::vec3 scale;
  glm::vec3 attachmentVector;
};

struct ControllerTranslationComponentEvent : public ComponentEvent {
  glm::vec3 controllerTranslationDelta;
};

struct ControllerRotationComponentEvent : public ComponentEvent {
  glm::vec3 controllerRotationDelta;
};

struct ActorDestroyedComponentEvent : public ComponentEvent {
  WActor* pActor = nullptr;
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
      return;
    }

    m_delegates[typeid(EventType)].emplace_back(std::bind(function, instance, std::placeholders::_1));
  }

  template<typename EventType, typename ClassType>
  void removeDelegate(void(ClassType::* function)(const ComponentEvent&)) {
    if (!function) {
      RE_LOG(Error, "Failed to remove the component event delegate, nullptr was received.");
      return;
    }

    std::type_index eventTypeId = typeid(EventType);

    if (m_delegates.contains(eventTypeId)) {
      uint32_t entryIndex = 0;
      auto& eventDelegates = m_delegates[eventTypeId];

      for (auto func : eventDelegates) {
        if (*func.target<decltype(function)>() == function) {
          eventDelegates.erase(eventDelegates.begin() + entryIndex);
          return;
        }

        ++entryIndex;
      }
    }
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