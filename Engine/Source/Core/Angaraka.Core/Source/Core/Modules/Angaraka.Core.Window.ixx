module;

#include "Angaraka/Base.hpp"

export module Angaraka.Core.Window;

namespace Angaraka
{
    // Forward declaration of the window handle type (actual HWND is in Windows.h)
    // We'll use a type alias to abstract it slightly.
    using WindowHandle = HWND;

    // Configuration struct for creating a window
    export struct WindowCreateInfo
    {
        std::wstring Title = L"Angaraka Engine";
        int Width = 1280;
        int Height = 720;
        bool Fullscreen = false;

        // More options can be added later: full screen, borderless, etc.
    };

    // Represents a single window in the engine
    export class Window
    {
    public:
        // Constructor: Creates and initializes the Win32 window
        Window();
        ~Window(); // Destructor: Destroys the Win32 window

        // Deleted copy constructor and assignment operator to prevent accidental copies
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        // Creates the actual Win32 window based on provided info
        bool Create(const WindowCreateInfo& createInfo);
        void Destroy();

        // Processes Windows messages. Returns false if a WM_QUIT message is received.
        bool ProcessMessages();

        // Getters
        WindowHandle GetHandle() const { return m_hWindow; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

        // Static callback for Win32 window procedure.
        // This is a requirement for Win32 API, we'll explain how to hook it up to our class instance.
        static LRESULT CALLBACK WindowProc(WindowHandle hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    private:
        WindowHandle m_hWindow = nullptr;
        int m_Width = 0;
        int m_Height = 0;
        // Store the instance handle for this application
        HINSTANCE m_hInst = nullptr;

        // Internal helper to register the window class
        bool RegisterWindowClass();
        // Internal helper to unregister the window class
        void UnregisterWindowClass();
    };

} // namespace Angaraka