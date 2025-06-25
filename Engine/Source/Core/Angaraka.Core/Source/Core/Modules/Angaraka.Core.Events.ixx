module;

#include "Angaraka/Base.hpp"

export module Angaraka.Core.Events;

namespace Angaraka::Events {
    export enum class EventCategory : U32 {
        None = 0,
        Application = (1 << 0),
        Input = (1 << 1),
        Keyboard = (1 << 2),
        Mouse = (1 << 3),
        Gamepad = (1 << 4),
        Physics = (1 << 5),
        Rendering = (1 << 6),
        Scripting = (1 << 7),
        // Add more categories as needed
        Dialogue = (1 << 8),
        Spawning = (1 << 9),
        NPCInteraction = (1 << 10),
    };

    export class Event {
    public:

        Event() = default;
        virtual ~Event() = default;

        // Pure virtual method to get the static type ID of the event
        virtual size_t GetStaticType() const = 0;

        // Virtual method to get the name of the event for debugging
        virtual const char* GetName() const = 0;

        // Virtual method to get the event category flags
        virtual int GetCategoryFlags() const = 0;

        // Helper to check if an event is in a specific category
        inline bool IsInCategory(EventCategory category) const {
            return GetCategoryFlags() & static_cast<int>(category);
        }

        static std::atomic<size_t> s_nextEventTypeId;
    };

    std::atomic<size_t> Event::s_nextEventTypeId = 1ULL;

    export template <typename T> inline size_t GetEventTypeId() {
        static const size_t id = std::atomic_fetch_add(&Event::s_nextEventTypeId, 1ULL);
        return id;
    }

    export template<typename T> requires (std::is_enum_v<T>) constexpr auto operator|(const T lhs, const T rhs) {
        return static_cast<T>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }

    // --- Event Listener Type ---
    using EventCallback = std::function<void(const Event&)>;
    using SubscriptionID = size_t; // Unique ID for each subscription

    // --- Event Manager Interface ---
    // This interface defines the core functionality of the event bus.
    export class IEventManager {
    public:
        virtual ~IEventManager() = default;

        // Subscribes a callback function to a specific event type.
        // The callback will be invoked when an event of that type is broadcast.
        template<typename EventType>
        inline SubscriptionID Subscribe(EventCallback callback) {
            AGK_INFO("EventManager: Subscribing callback to event type: {0}", typeid(EventType).name());
            return SubscribeInternal(EventType::GetStaticType_s(), callback);
        }

        // Unsubscribes a specific callback from an event type using its SubscriptionID.
        template<typename EventType>
        inline void Unsubscribe(SubscriptionID id) {
            AGK_INFO("EventManager: Unsubscribing callback from event type: {0}", typeid(EventType).name());
            UnsubscribeInternal(EventType::GetStaticType_s(), id);
        }

        // Broadcasts an event to all subscribed listeners.
        // The event object is passed by const reference.
        template<typename EventType>
        inline void Broadcast(const EventType& event) {
            AGK_TRACE("EventManager: Broadcasting event: {0}", event.GetName());
            BroadcastInternal(event.GetStaticType(), event);
        }

    protected:
        // Internal methods to be implemented by concrete EventManager.
        // Using size_t for type ID to keep map type-agnostic.
        virtual SubscriptionID SubscribeInternal(size_t typeId, EventCallback callback) = 0;
        virtual void UnsubscribeInternal(size_t typeId, SubscriptionID id) = 0;
        virtual void BroadcastInternal(size_t typeId, const Event& event) = 0;
    };


    // --- Concrete Event Manager Implementation ---
    // This class provides the actual implementation of the event bus.
    export class EventManager : public IEventManager {
    public:
        EventManager();
        virtual ~EventManager();

        // Singleton accessor (simple for now, can be integrated with a PluginManager later)
        static EventManager& Get();

    protected: // Protected as per IEventManager, but also allows direct calls from within the module if needed
        SubscriptionID SubscribeInternal(size_t typeId, EventCallback callback) override;
        void UnsubscribeInternal(size_t typeId, SubscriptionID id) override;
        void BroadcastInternal(size_t typeId, const Event& event) override;

    private:
        // Map to store event listeners: Event Type ID -> (SubscriptionID -> Callback)
        // Using a map for inner storage to allow easy removal by ID.
        std::map<size_t, std::map<SubscriptionID, EventCallback>> m_listeners;
        static Scope<EventManager> s_instance; // Singleton instance
        std::atomic<SubscriptionID> m_nextSubscriptionId; // For generating unique IDs
    };


}