#pragma once

#include "pch.h"

#define MAX_COMPONENT_EVENTS 64

class ABase;
struct WComponent;

struct ComponentEvent {
  ABase* pEventOwner = nullptr;

  virtual ~ComponentEvent() {}
};

struct TransformUpdateComponentEvent : public ComponentEvent {
  glm::vec3 translation;
};

class ComponentEventSystem {
  using ComponentDelegate = std::function<void(const ComponentEvent&)>;

private:
  // Map: [event type] - vector of [WComponent]
  std::unordered_map<std::type_index, std::vector<ComponentDelegate>> m_delegates;
  std::array<ComponentEvent, MAX_COMPONENT_EVENTS> m_queuedEvents;
  uint32_t m_eventIndex = 0u;

public:
  template<typename EventType, typename ClassType>
  void addDelegate(ClassType* instance, void(ClassType::*function)(const ComponentEvent&)) {
    if (!function || !instance) {
      RE_LOG(Error, "Couldn't add component event delegate, nullptr was received.");
    }

    m_delegates[typeid(EventType)].emplace_back(std::bind(function, instance, std::placeholders::_1));
  }

  template<typename EventType>
  void addEvent(const EventType& newEvent) {
    if (m_eventIndex < MAX_COMPONENT_EVENTS) {
      m_queuedEvents[m_eventIndex] = newEvent;
      ++m_eventIndex;
      return;
    }

    RE_LOG(Error, "Maximum component events reached.");
  }

  void processEvents() {
    for (; m_eventIndex > 0; --m_eventIndex) {
      std::type_index eventTypeId = typeid(m_queuedEvents[m_eventIndex - 1]);

      if (m_delegates.contains(eventTypeId)) {
        for (auto& func : m_delegates[eventTypeId]) {
          func(m_queuedEvents[m_eventIndex - 1]);
        }
      }
    }
  }
};