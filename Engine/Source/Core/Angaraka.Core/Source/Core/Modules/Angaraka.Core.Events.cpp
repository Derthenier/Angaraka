module;

#include "Angaraka/Base.hpp"
#include "Angaraka/Log.hpp"
#include <map>

module Angaraka.Core.Events;

namespace Angaraka::Events {

    namespace {

    }

    std::unique_ptr<EventManager> EventManager::s_instance = nullptr;

    EventManager::EventManager() : m_nextSubscriptionId(1ULL) {
        AGK_INFO("EventManager: Constructor called.");
        // Ensure only one instance is created
        if (s_instance) {
            AGK_ERROR("EventManager: Attempted to create a second EventManager instance!");
            throw std::runtime_error("Only one EventManager instance is allowed.");
        }
    }

    EventManager::~EventManager() {
        AGK_INFO("EventManager: Destructor called.");
        m_listeners.clear(); // Clear all listeners
    }

    EventManager& EventManager::Get() {
        // Create the instance lazily if it doesn't exist
        if (!s_instance) {
            s_instance = std::unique_ptr<EventManager>(new EventManager()); // Use new to create and manage lifetime
        }
        return *s_instance;
    }

    SubscriptionID EventManager::SubscribeInternal(size_t typeId, EventCallback callback) {
        SubscriptionID newId = std::atomic_fetch_add(&m_nextSubscriptionId, 1ULL);
        m_listeners[typeId][newId] = callback; // Store callback with its new ID
        AGK_INFO("EventManager: Listener added for event type ID: {0}, Subscription ID: {1}", typeId, newId);
        return newId;
    }

    void EventManager::UnsubscribeInternal(size_t typeId, SubscriptionID id) {
        auto it = m_listeners.find(typeId);
        if (it != m_listeners.end()) {
            size_t erased_count = it->second.erase(id); // Erase by SubscriptionID
            if (erased_count > 0) {
                AGK_INFO("EventManager: Listener removed for event type ID: {0}, Subscription ID: {1}", typeId, id);
            }
            else {
                AGK_WARN("EventManager: Attempted to unsubscribe non-existent listener for event type ID: {0}, Subscription ID: {1}", typeId, id);
            }
            // If no more listeners for this event type, remove the inner map
            if (it->second.empty()) {
                m_listeners.erase(it);
                AGK_INFO("EventManager: All listeners for event type ID: {0} removed. Removing type entry.", typeId);
            }
        }
        else {
            AGK_WARN("EventManager: Attempted to unsubscribe from non-existent event type ID: {0}", typeId);
        }
    }

    void EventManager::BroadcastInternal(size_t typeId, const Event& event) {
        // Find listeners for this event type
        auto it = m_listeners.find(typeId);
        if (it != m_listeners.end()) {
            // Iterate through all callbacks and invoke them
            // Create a copy of the callbacks for thread-safety during iteration
            // (prevents issues if a callback unsubscribes itself during iteration)
            std::vector<EventCallback> callbacksToInvoke;
            callbacksToInvoke.reserve(it->second.size());
            for (const auto& pair : it->second) {
                callbacksToInvoke.push_back(pair.second);
            }

            for (const auto& callback : callbacksToInvoke) {
                callback(event);
            }
        }
    }


}