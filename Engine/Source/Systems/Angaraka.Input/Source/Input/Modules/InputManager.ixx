module;

#include "Angaraka/InputBase.hpp"
#include <stdexcept>

// --- Helper Macro for Event Declaration ---
// This macro helps in boilerplate reduction for concrete event classes.
#define EVENT_CLASS_TYPE(type)                                                       \
inline static size_t GetStaticType_s() { return Angaraka::Events::GetEventTypeId<type>(); } \
inline size_t GetStaticType() const override { return GetStaticType_s(); }                  \
inline const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) inline int GetCategoryFlags() const override { return static_cast<int>(category); }

export module Angaraka.Input.Windows;

import Angaraka.Input.Windows.Events;

namespace Angaraka::Input {
    export class Win64InputSystem {
    public:
        Win64InputSystem();
        ~Win64InputSystem();

        bool Initialize(HWND windowHandle);
        // Update now primarily responsible for broadcasting events
        void Update();

        // Query methods (will be deprecated in favor of events for many uses)
        bool IsKeyDown(int virtualKeyCode) const;
        bool IsMouseButtonDown(int virtualKeyCode) const;
        MouseDelta GetMouseDelta() const; // Will be derived from MouseMovedEvent data

        void SetMouseCursorVisible(bool visible);
        void SetMouseCursorConfined(bool confined);

    private:
        HWND m_windowHandle = nullptr;
        bool m_mouseCursorVisible = true;
        bool m_mouseCursorConfined = false;

        // Mouse state for delta calculation (still needed internally by InputManager to *produce* events)
        bool m_firstMouse = true;
        F32 m_lastMouseX = 0.0f;
        F32 m_lastMouseY = 0.0f;

        // Key states for tracking presses/releases (needed to fire events only on state change)
        bool m_keyStates[256]; // Using 256 for VK_ keys
        bool m_mouseButtonStates[3]; // VK_LBUTTON, VK_RBUTTON, VK_MBUTTON
    };
}