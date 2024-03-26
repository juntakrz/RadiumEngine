#pragma once

#include "pch.h"

#define MAX_COMPONENT_EVENTS 64

struct WComponent;

struct ComponentEvent {
  virtual ~ComponentEvent() {}
};

struct TransformUpdateComponentEvent : public ComponentEvent {
  glm::vec3 translation;
};

class ComponentEventSystem {
private:
  // Map: [event type] - vector of [WComponent]
  std::unordered_map<std::type_index, std::vector<WComponent*>> m_subscribers;
  std::array<ComponentEvent, MAX_COMPONENT_EVENTS> m_queuedEvents;
  uint32_t m_eventIndex = 0u;

public:
  template<typename EventType>
  void addListener(WComponent* pListener) {
    m_subscribers[typeid(EventType)].emplace_back(pListener);
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

      if (m_subscribers.contains(eventTypeId)) {
        for (auto& subscriber : m_subscribers[eventTypeId]) {
          subscriber->onEvent(m_queuedEvents[m_eventIndex - 1]);
        }
      }
    }
  }
};