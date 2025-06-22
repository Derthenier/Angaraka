module;

#include <Angaraka/Base.hpp>

module ThreadsOfKaliyuga.Input;

import Angaraka.Camera;
import Angaraka.Core.Events;
import Angaraka.Input.Windows;

namespace ThreadsOfKaliyuga
{
    bool InputSystem::Initialize(HWND windowHandle, Angaraka::Camera* camera, bool fullscreen)
    {
        m_inputManager = new Angaraka::Input::Win64InputSystem();
        if (!m_inputManager->Initialize(windowHandle)) {
            AGK_APP_FATAL("Failed to initialize input system. Stopping engine...");
            return false;
        }
        m_inputManager->SetMouseCursorVisible(true);
        m_inputManager->SetMouseCursorConfined(fullscreen);

        m_camera = camera;
        return true;
    }

    void InputSystem::Shutdown()
    {
        // --- Explicitly Unsubscribe from Events to prevent crash on exit ---
        // These checks prevent trying to unsubscribe with invalid IDs (e.g., if init failed)
        if (m_keyPressedSubId != 0) {
            Angaraka::Events::EventManager::Get().Unsubscribe<Angaraka::Input::KeyPressedEvent>(m_keyPressedSubId);
            m_keyPressedSubId = 0; // Reset ID after unsubscribing
        }
        if (m_keyReleasedSubId != 0) {
            Angaraka::Events::EventManager::Get().Unsubscribe<Angaraka::Input::KeyReleasedEvent>(m_keyReleasedSubId);
            m_keyReleasedSubId = 0;
        }
        if (m_mouseMovedSubId != 0) {
            Angaraka::Events::EventManager::Get().Unsubscribe<Angaraka::Input::MouseMovedEvent>(m_mouseMovedSubId);
            m_mouseMovedSubId = 0;
        }
        if (m_mouseButtonPressedSubId != 0) {
            Angaraka::Events::EventManager::Get().Unsubscribe<Angaraka::Input::MouseButtonPressedEvent>(m_mouseButtonPressedSubId);
            m_mouseButtonPressedSubId = 0;
        }
        if (m_mouseButtonReleasedSubId != 0) {
            Angaraka::Events::EventManager::Get().Unsubscribe<Angaraka::Input::MouseButtonReleasedEvent>(m_mouseButtonReleasedSubId);
            m_mouseButtonReleasedSubId = 0;
        }

        if (m_inputManager) {
            delete m_inputManager;
            m_inputManager = nullptr;
        }
        AGK_APP_INFO("InputManager shutdown.");
    }

    void InputSystem::SetupEvents() {

        // Keyboard Input Subscriptions
        m_keyPressedSubId = Angaraka::Events::EventManager::Get().Subscribe<Angaraka::Input::KeyPressedEvent>(
            [this](const Angaraka::Events::Event& event) {
                const auto& keyEvent = static_cast<const Angaraka::Input::KeyPressedEvent&>(event);
                AGK_APP_TRACE("KeyPressedEvent: KeyCode={0}", keyEvent.GetKeyCode());

                // Set camera movement flags
                switch (keyEvent.GetKeyCode()) {
                case 'W': m_camera->SetMoveState(Angaraka::Camera_Movement::FORWARD, true); break;
                case 'S': m_camera->SetMoveState(Angaraka::Camera_Movement::BACKWARD, true); break;
                case 'A': m_camera->SetMoveState(Angaraka::Camera_Movement::LEFT, true); break;
                case 'D': m_camera->SetMoveState(Angaraka::Camera_Movement::RIGHT, true); break;
                case VK_SPACE: m_camera->SetMoveState(Angaraka::Camera_Movement::UP, true); break;
                case VK_LCONTROL: m_camera->SetMoveState(Angaraka::Camera_Movement::DOWN, true); break;
                case VK_ESCAPE: m_camera->Reset(); break;
                }
            }
        );

        m_keyReleasedSubId = Angaraka::Events::EventManager::Get().Subscribe<Angaraka::Input::KeyReleasedEvent>(
            [this](const Angaraka::Events::Event& event) {
                const auto& keyEvent = static_cast<const Angaraka::Input::KeyReleasedEvent&>(event);
                AGK_APP_TRACE("KeyReleasedEvent: KeyCode={0}", keyEvent.GetKeyCode());

                // Clear camera movement flags
                switch (keyEvent.GetKeyCode()) {
                case 'W': m_camera->SetMoveState(Angaraka::Camera_Movement::FORWARD, false); break;
                case 'S': m_camera->SetMoveState(Angaraka::Camera_Movement::BACKWARD, false); break;
                case 'A': m_camera->SetMoveState(Angaraka::Camera_Movement::LEFT, false); break;
                case 'D': m_camera->SetMoveState(Angaraka::Camera_Movement::RIGHT, false); break;
                case VK_SPACE: m_camera->SetMoveState(Angaraka::Camera_Movement::UP, false); break;
                case VK_LCONTROL: m_camera->SetMoveState(Angaraka::Camera_Movement::DOWN, false); break;
                case VK_ESCAPE: m_camera->Reset(); break;
                }
            }
        );

        // Mouse Movement Subscription
        m_mouseMovedSubId = Angaraka::Events::EventManager::Get().Subscribe<Angaraka::Input::MouseMovedEvent>(
            [this](const Angaraka::Events::Event& event) {
                const auto& mouseEvent = static_cast<const Angaraka::Input::MouseMovedEvent&>(event);
                // Only process mouse movement for camera when right mouse button is held down
                // (or if your game has a permanent 'look' mode).
                if (m_inputManager->IsMouseButtonDown(VK_RBUTTON)) {
                    m_camera->ProcessMouseMovement(mouseEvent.GetX(), mouseEvent.GetY());
                }
            }
        );

        // Mouse Button Subscriptions (for general use, not directly for camera movement in this pattern)
        m_mouseButtonPressedSubId = Angaraka::Events::EventManager::Get().Subscribe<Angaraka::Input::MouseButtonPressedEvent>(
            [this](const Angaraka::Events::Event& event) {
                const auto& mouseEvent = static_cast<const Angaraka::Input::MouseButtonPressedEvent&>(event);
                AGK_APP_TRACE("MouseButtonPressedEvent: Button={0}", mouseEvent.GetMouseButton());
            }
        );

        m_mouseButtonReleasedSubId = Angaraka::Events::EventManager::Get().Subscribe<Angaraka::Input::MouseButtonReleasedEvent>(
            [this](const Angaraka::Events::Event& event) {
                const auto& mouseEvent = static_cast<const Angaraka::Input::MouseButtonReleasedEvent&>(event);
                AGK_APP_TRACE("MouseButtonReleasedEvent: Button={0}", mouseEvent.GetMouseButton());
            }
        );

    }

    void InputSystem::Update(Angaraka::F32 deltaTime)
    {
        if (m_inputManager) {
            m_inputManager->Update();
        }
    }
}