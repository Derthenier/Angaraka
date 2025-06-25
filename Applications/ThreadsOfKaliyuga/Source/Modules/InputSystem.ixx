module;

#include <Angaraka/Base.hpp>

export module ThreadsOfKaliyuga.Input;

import Angaraka.Core.Events;
import Angaraka.Input.Windows;
import Angaraka.Camera;

namespace ThreadsOfKaliyuga
{
    export class InputSystem final
    {
    public:
        InputSystem() = default;
        ~InputSystem() { Shutdown(); }

        bool Initialize(HWND windowHandle, Angaraka::Camera* camera, bool fullscreen);
        void Shutdown();

        void Update(Angaraka::F32 deltaTime);
        void SetupEvents();

    private:
        Angaraka::Input::Win64InputSystem* m_inputManager;
        Angaraka::Camera* m_camera;

        size_t m_keyPressedSubId{ 0ULL };
        size_t m_keyReleasedSubId{ 0ULL };
        size_t m_mouseMovedSubId{ 0ULL };
        size_t m_mouseButtonPressedSubId{ 0ULL };
        size_t m_mouseButtonReleasedSubId{ 0ULL };

    };
}