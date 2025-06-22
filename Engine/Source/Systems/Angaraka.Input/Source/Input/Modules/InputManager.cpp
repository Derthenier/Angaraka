module;

#include "Angaraka/InputBase.hpp"

module Angaraka.Input.Windows;

import Angaraka.Core.Events;
import Angaraka.Input.Windows.Events;

namespace Angaraka::Input {

    namespace {

    }

    Win64InputSystem::Win64InputSystem() {
        AGK_INFO("Win64InputSystem: Constructor called.");
        // Initialize key and mouse button states to false
        std::fill(std::begin(m_keyStates), std::end(m_keyStates), false);
        std::fill(std::begin(m_mouseButtonStates), std::end(m_mouseButtonStates), false);
    }

    Win64InputSystem::~Win64InputSystem() {
        AGK_INFO("Win64InputSystem: Destructor called.");
        if (!m_mouseCursorVisible) {
            SetMouseCursorVisible(true);
        }
        if (m_mouseCursorConfined) {
            ClipCursor(NULL); // Release mouse from confinement
        }
    }

    bool Win64InputSystem::Initialize(HWND windowHandle) {
        if (windowHandle == nullptr) {
            AGK_ERROR("Win64InputSystem: Initialize failed. Invalid window handle.");
            return false;
        }
        m_windowHandle = windowHandle;
        AGK_INFO("Win64InputSystem: Initialized for window handle {0}.", (void*)m_windowHandle);

        // Optional: Hide and confine mouse by default for game
        SetMouseCursorVisible(false);
        SetMouseCursorConfined(true); // Call this after window is fully created and focused

        return true;
    }

    void Win64InputSystem::Update() {
        // --- Process Keyboard Input and Broadcast Events ---
        for (int i = 0; i < 256; ++i) { // Iterate through all possible virtual key codes
            bool isCurrentlyDown = (GetAsyncKeyState(i) & 0x8000) != 0;

            if (isCurrentlyDown && !m_keyStates[i]) {
                // Key was just pressed
                // Note: GetAsyncKeyState doesn't give repeat count directly for key presses.
                // For accurate repeat count, you'd process WM_KEYDOWN messages.
                // Here, we'll just use a placeholder for repeatCount.
                KeyPressedEvent event(i, 0); // 0 for repeat count
                Angaraka::Events::EventManager::Get().Broadcast(event);
                m_keyStates[i] = true;
            }
            else if (!isCurrentlyDown && m_keyStates[i]) {
                // Key was just released
                KeyReleasedEvent event(i);
                Angaraka::Events::EventManager::Get().Broadcast(event);
                m_keyStates[i] = false;
            }
        }

        // --- Process Mouse Button Input and Broadcast Events ---
        int mouseButtons[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
        for (int i = 0; i < _countof(mouseButtons); ++i) {
            int buttonCode = mouseButtons[i];
            bool isCurrentlyDown = (GetAsyncKeyState(buttonCode) & 0x8000) != 0;

            if (isCurrentlyDown && !m_mouseButtonStates[i]) {
                // Mouse button was just pressed
                MouseButtonPressedEvent event(buttonCode);
                Angaraka::Events::EventManager::Get().Broadcast(event);
                m_mouseButtonStates[i] = true;
            }
            else if (!isCurrentlyDown && m_mouseButtonStates[i]) {
                // Mouse button was just released
                MouseButtonReleasedEvent event(buttonCode);
                Angaraka::Events::EventManager::Get().Broadcast(event);
                m_mouseButtonStates[i] = false;
            }
        }


        // --- Process Mouse Movement and Broadcast Events ---
        POINT currentMousePos;
        if (!GetCursorPos(&currentMousePos)) {
            AGK_WARN("Win64InputSystem: Failed to get cursor position.");
            return;
        }
        if (!ScreenToClient(m_windowHandle, &currentMousePos)) {
            AGK_WARN("Win64InputSystem: Failed to convert screen to client coordinates.");
            return;
        }

        if (m_firstMouse) {
            m_lastMouseX = (F32)currentMousePos.x;
            m_lastMouseY = (F32)currentMousePos.y;
            m_firstMouse = false;
        }

        F32 deltaX = (F32)currentMousePos.x - m_lastMouseX;
        F32 deltaY = m_lastMouseY - (F32)currentMousePos.y; // Invert Y-axis

        if (deltaX != 0.0f || deltaY != 0.0f) {
            // Broadcast MouseMovedEvent with delta
            MouseMovedEvent event(deltaX, deltaY); // Using delta for convenience of camera
            Angaraka::Events::EventManager::Get().Broadcast(event);
        }

        m_lastMouseX = (F32)currentMousePos.x;
        m_lastMouseY = (F32)currentMousePos.y;

        // If mouse is confined, recenter it to prevent it hitting screen edges
        // This makes delta calculation simpler as cursor is always near center
        if (m_mouseCursorConfined) {
            RECT clientRect;
            GetClientRect(m_windowHandle, &clientRect);
            POINT center;
            center.x = (clientRect.left + clientRect.right) / 2;
            center.y = (clientRect.top + clientRect.bottom) / 2;
            ClientToScreen(m_windowHandle, &center);
            SetCursorPos(center.x, center.y);
            m_lastMouseX = (F32)((clientRect.left + clientRect.right) / 2);
            m_lastMouseY = (F32)((clientRect.top + clientRect.bottom) / 2);
        }

        // Mouse scroll events usually come from WM_MOUSEWHEEL, not GetAsyncKeyState.
        // For a full event system, you'd process WM_MOUSEWHEEL in your window procedure
        // and broadcast MouseScrolledEvent from there. For now, we'll omit it from Update().
    }

    // These query methods can still be used, but the primary way to react
    // to input will now be through event subscriptions.
    bool Win64InputSystem::IsKeyDown(int virtualKeyCode) const {
        return m_keyStates[virtualKeyCode]; // Return internal state, not poll GetAsyncKeyState again
    }

    bool Win64InputSystem::IsMouseButtonDown(int virtualKeyCode) const {
        if (virtualKeyCode == VK_LBUTTON) return m_mouseButtonStates[0];
        if (virtualKeyCode == VK_RBUTTON) return m_mouseButtonStates[1];
        if (virtualKeyCode == VK_MBUTTON) return m_mouseButtonStates[2];
        return false;
    }

    MouseDelta Win64InputSystem::GetMouseDelta() const {
        // This method becomes less relevant if all mouse movement is event-driven.
        // For now, it just returns the last calculated delta from Update().
        // In a true event-driven model, consumers would get this from MouseMovedEvent directly.
        AGK_WARN("Win64InputSystem::GetMouseDelta() is deprecated. Use MouseMovedEvent via EventBus instead.");
        return MouseDelta{}; // Return empty delta as it's not the primary way now
    }

    void Win64InputSystem::SetMouseCursorVisible(bool visible) {
        if (m_mouseCursorVisible != visible) {
            ShowCursor(visible);
            m_mouseCursorVisible = visible;
            AGK_INFO("Win64InputSystem: Mouse cursor visibility set to {0}.", visible ? "visible" : "hidden");
        }
    }

    void Win64InputSystem::SetMouseCursorConfined(bool confined) {
        if (m_mouseCursorConfined != confined) {
            if (confined) {
                RECT clientRect;
                GetClientRect(m_windowHandle, &clientRect);
                MapWindowPoints(m_windowHandle, NULL, (LPPOINT)&clientRect, 2); // Convert client rect to screen rect
                ClipCursor(&clientRect);
                AGK_INFO("Win64InputSystem: Mouse cursor confined to window.");
            }
            else {
                ClipCursor(NULL); // Release mouse from confinement
                AGK_INFO("Win64InputSystem: Mouse cursor confinement released.");
            }
            m_mouseCursorConfined = confined;
        }
    }

}