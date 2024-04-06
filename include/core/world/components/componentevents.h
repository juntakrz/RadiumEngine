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

class ComponentDelegate_Base {
public:
  virtual bool compareOwner(void*) = 0;
  virtual void execute(const ComponentEvent&) = 0;
  virtual void operator()(const ComponentEvent&) = 0;
};

template <typename ClassType>
class ComponentDelegate : public ComponentDelegate_Base {
public:
  ClassType* pOwner = nullptr;
  void(ClassType::* function)(const ComponentEvent&) = nullptr;

  ComponentDelegate(ClassType* pNewOwner, void(ClassType::* newFunction)(const ComponentEvent&))
    : pOwner(pNewOwner), function(newFunction) {};

  bool compareOwner(void* pObject) override {
    return pObject == static_cast<void*>(pOwner);
  }

  void execute(const ComponentEvent& newEvent) override {
    return (pOwner->*function)(newEvent);
  };

  void operator()(const ComponentEvent& newEvent) override {
    return (pOwner->*function)(newEvent);
  };
};

class ComponentEventSystem {
private:
  // Map: [event type] - vector of [WComponent]
  std::unordered_map<std::type_index, std::vector<std::unique_ptr<ComponentDelegate_Base>>> m_delegates;

public:
  template<typename EventType, typename ClassType>
  void addDelegate(ClassType* instance, void(ClassType::* function)(const ComponentEvent&)) {
    if (!function || !instance) {
      RE_LOG(Error, "Couldn't add component event delegate, nullptr was received.");
      return;
    }

    m_delegates[typeid(EventType)].emplace_back(std::make_unique<ComponentDelegate<ClassType>>(instance, function));
  }

  template<typename EventType>
  void removeDelegates(void* pOwner) {
    if (!pOwner) {
      RE_LOG(Error, "Failed to remove the component event delegate, nullptr was received.");
      return;
    }

    std::type_index eventTypeId = typeid(EventType);

    if (m_delegates.contains(eventTypeId)) {
      uint32_t entryIndex = 0;
      auto& eventDelegates = m_delegates[eventTypeId];

      for (const auto& eventDelegate : eventDelegates) {
        if (eventDelegate->compareOwner(pOwner)) {
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
      for (auto& eventDelegate : m_delegates[eventTypeId]) {
        eventDelegate->execute(newEvent);
      }
    }
  }
};