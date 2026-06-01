#pragma once

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

struct IEvent {
    virtual ~IEvent() = default;
};

class EventBus {
public:
    template <typename EventType>
    using EventHandler = std::function<void(const EventType&)>;

    template <typename EventType>
    void Subscribe(EventHandler<EventType> handler) {
        const std::type_index eventType = std::type_index(typeid(EventType));

        subscribers[eventType].push_back(
            [handler](const IEvent& event) {
                handler(static_cast<const EventType&>(event));
            }
        );
    }

    template <typename EventType>
    void Publish(const EventType& event) const {
        const std::type_index eventType = std::type_index(typeid(EventType));
        const auto iterator = subscribers.find(eventType);

        if (iterator == subscribers.end()) {
            return;
        }

        for (const auto& subscriber : iterator->second) {
            subscriber(event);
        }
    }

    void Clear() {
        subscribers.clear();
    }

private:
    using SubscriberCallback = std::function<void(const IEvent&)>;
    std::unordered_map<std::type_index, std::vector<SubscriberCallback>> subscribers;
};
