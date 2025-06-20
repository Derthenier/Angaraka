module;

#include "Angaraka/InputBase.hpp"

// --- Helper Macro for Event Declaration ---
// This macro helps in boilerplate reduction for concrete event classes.
#define EVENT_CLASS_TYPE(type)                                                       \
inline static size_t GetStaticType_s() { return Angaraka::Events::GetEventTypeId<type>(); } \
inline size_t GetStaticType() const override { return GetStaticType_s(); }                  \
inline const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) inline int GetCategoryFlags() const override { return static_cast<int>(category); }

export module Angaraka.Input.Windows.Events;

import Angaraka.Core.Events;

namespace Angaraka::Input {

    // Helper struct for mouse delta (already exists, but confirming placement)
    export struct MouseDelta {
        float x = 0.0f;
        float y = 0.0f;
        float scroll = 0.0f;
    };

    // --- Input Event Definitions ---

    export class KeyboardEvent : public Angaraka::Events::Event {
    public:
        // Base class for all keyboard events
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Input | Angaraka::Events::EventCategory::Keyboard)
        inline int GetKeyCode() const { return m_keyCode; }
    protected:
        inline KeyboardEvent(int keyCode) : m_keyCode(keyCode) {}
        int m_keyCode;
    };

    export class KeyPressedEvent : public KeyboardEvent {
    public:
        inline KeyPressedEvent(int keyCode, int repeatCount)
            : KeyboardEvent(keyCode), m_repeatCount(repeatCount) {
        }

        inline int GetRepeatCount() const { return m_repeatCount; }
        EVENT_CLASS_TYPE(KeyPressedEvent)
    private:
        int m_repeatCount;
    };

    export class KeyReleasedEvent : public KeyboardEvent {
    public:
        inline KeyReleasedEvent(int keyCode)
            : KeyboardEvent(keyCode) {
        }

        EVENT_CLASS_TYPE(KeyReleasedEvent)
    };

    // --- Mouse Events ---

    export class MouseMovedEvent : public Angaraka::Events::Event {
    public:
        inline MouseMovedEvent(float x, float y)
            : m_mouseX(x), m_mouseY(y) {
        }

        inline float GetX() const { return m_mouseX; }
        inline float GetY() const { return m_mouseY; }

        EVENT_CLASS_TYPE(MouseMovedEvent)
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Input | Angaraka::Events::EventCategory::Mouse)
    private:
        float m_mouseX, m_mouseY; // Absolute position or delta depending on implementation
    };

    export class MouseScrolledEvent : public Angaraka::Events::Event {
    public:
        inline MouseScrolledEvent(float xOffset, float yOffset)
            : m_xOffset(xOffset), m_yOffset(yOffset) {
        }

        inline float GetXOffset() const { return m_xOffset; }
        inline float GetYOffset() const { return m_yOffset; }

        EVENT_CLASS_TYPE(MouseScrolledEvent)
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Input | Angaraka::Events::EventCategory::Mouse)
    private:
        float m_xOffset, m_yOffset; // Scroll delta
    };

    export class MouseButtonEvent : public Angaraka::Events::Event {
    public:
        inline int GetMouseButton() const { return m_button; }
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Input | Angaraka::Events::EventCategory::Mouse)
    protected:
        inline MouseButtonEvent(int button) : m_button(button) {}
        int m_button;
    };

    export class MouseButtonPressedEvent : public MouseButtonEvent {
    public:
        inline MouseButtonPressedEvent(int button)
            : MouseButtonEvent(button) {
        }

        EVENT_CLASS_TYPE(MouseButtonPressedEvent)
    };

    export class MouseButtonReleasedEvent : public MouseButtonEvent {
    public:
        inline MouseButtonReleasedEvent(int button)
            : MouseButtonEvent(button) {
        }

        EVENT_CLASS_TYPE(MouseButtonReleasedEvent)
    };
}