module;

#include "Angaraka/Base.hpp"
#include "Angaraka/Log.hpp"
#include <map>

module Angaraka.Core.Window;

namespace Angaraka
{
    namespace {
        // Static map to associate HWNDs with Window instances.
        // This is a common pattern for routing Win32 messages to C++ class instances.
        std::map<WindowHandle, Window*> s_WindowMap;
        const WCHAR* s_WindowClassName = L"AngarakaWindowClass";
    }

    Window::Window()
    {
        // Get the instance handle of the current module/executable
        m_hInst = GetModuleHandleW(nullptr);
        if (m_hInst == nullptr)
        {
            AGK_FATAL("Failed to get module handle for window creation!");
            throw std::runtime_error("Failed to get module handle.");
        }
    }

    Window::~Window()
    {
        Destroy(); // Ensure window is destroyed on object destruction
    }

    bool Window::RegisterWindowClass()
    {
        // Check if the class is already registered (e.g., if multiple windows use the same class)
        WNDCLASSEXW wc = { 0 };
        if (GetClassInfoExW(m_hInst, s_WindowClassName, &wc))
        {
            // Class already registered, nothing to do
            return true;
        }

        // Fill out the window class structure
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // Redraw on resize, own device context
        wc.lpfnWndProc = WindowProc; // Our static window procedure
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = m_hInst;
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION); // Default application icon
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);      // Default arrow cursor
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Default background color
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = s_WindowClassName;
        wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION); // Small icon

        if (!RegisterClassExW(&wc))
        {
            AGK_ERROR("Failed to register window class! Error: {:#x}", GetLastError());
            return false;
        }

        AGK_INFO(L"Window class '{}' registered successfully.", s_WindowClassName);
        return true;
    }

    void Window::UnregisterWindowClass()
    {
        if (m_hInst != nullptr)
        {
            if (!UnregisterClassW(s_WindowClassName, m_hInst))
            {
                AGK_WARN(L"Failed to unregister window class '{}'! Error: {:#x}", s_WindowClassName, GetLastError());
            }
            else
            {
                AGK_INFO(L"Window class '{}' unregistered.", s_WindowClassName);
            }
        }
    }

    bool Window::Create(const WindowCreateInfo& createInfo)
    {
        m_Width = createInfo.Width;
        m_Height = createInfo.Height;

        if (!RegisterWindowClass())
        {
            return false;
        }

        auto style = createInfo.Fullscreen ? WS_POPUP | WS_VISIBLE : WS_OVERLAPPEDWINDOW;

        // Create the actual window instance
        m_hWindow = CreateWindowExW(
            0,                        // Optional window styles
            s_WindowClassName,        // Window class name
            createInfo.Title.c_str(), // Window title
            style,                    // Window style (standard title bar, resizable, min/max buttons)
            0, 0,                     // Position (x, y)
            m_Width, m_Height,        // Size (width, height)
            nullptr,                  // Parent window handle
            nullptr,                  // Menu handle
            m_hInst,                  // Instance handle
            this                      // Pointer to our 'this' object, passed to WM_NCCREATE
        );

        if (m_hWindow == nullptr)
        {
            AGK_ERROR("Failed to create window! Error: {:#x}", GetLastError());
            UnregisterWindowClass(); // Clean up if window creation fails
            return false;
        }

        // Associate the HWND with this Angaraka::Window instance
        s_WindowMap[m_hWindow] = this;

        // Show the window
        ShowWindow(m_hWindow, (createInfo.Fullscreen ? SW_SHOWMAXIMIZED : SW_SHOW));
        UpdateWindow(m_hWindow); // Paint the window immediately

        AGK_INFO(L"Window '{:s}' created successfully. Size: {}x{}", createInfo.Title, m_Width, m_Height);
        return true;
    }

    void Window::Destroy()
    {
        if (m_hWindow != nullptr)
        {
            // Safety check: only erase if the window exists in map
            auto it = s_WindowMap.find(m_hWindow);
            if (it != s_WindowMap.end() && it->second == this) {
                s_WindowMap.erase(it);
            }

            if (!DestroyWindow(m_hWindow))
            {
                AGK_WARN("Failed to destroy window! Error: {:#x}", GetLastError());
            }
            else
            {
                AGK_INFO("Window destroyed.");
            }
            m_hWindow = nullptr;
        }
    }

    bool Window::ProcessMessages()
    {
        MSG msg = { 0 };
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); // Translates virtual-key messages into character messages
            DispatchMessageW(&msg);   // Dispatches a message to a window procedure

            if (msg.message == WM_QUIT)
            {
                AGK_INFO("Received WM_QUIT message. Exiting message loop.");
                return false; // Signal that the application should quit
            }
        }
        return true; // Continue processing messages
    }

    // Static Window Procedure
    LRESULT CALLBACK Window::WindowProc(WindowHandle hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Window* pWindow = nullptr;

        // When WM_NCCREATE is sent, the 'lParam' contains a pointer to the CREATESTRUCT.
        // Inside CREATESTRUCT, the 'lpCreateParams' member points to the 'this' pointer
        // we passed in CreateWindowEx. This is how we hook up the Win32 window to our C++ object.
        if (message == WM_NCCREATE)
        {
            CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pWindow = reinterpret_cast<Window*>(pCreate->lpCreateParams);
            if (pWindow)
            {
                // Store the 'this' pointer in the window's user data for future messages
                SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
                s_WindowMap[hWnd] = pWindow; // Also update our map
            }
        }
        else
        {
            // For all other messages, retrieve the 'this' pointer from the window's user data
            // or from our static map
            auto it = s_WindowMap.find(hWnd);
            if (it != s_WindowMap.end())
            {
                pWindow = it->second;
            }
            // If not found in map, try user data (should ideally be consistent)
            if (!pWindow)
            {
                pWindow = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            }
        }

        if (pWindow)
        {
            // Delegate to a non-static member function for message handling if needed.
            // For now, we'll handle basic messages directly here.
            switch (message)
            {
            case WM_CLOSE:
            {
                AGK_INFO("WM_CLOSE received. Posting WM_QUIT.");
                PostQuitMessage(0); // This will cause PeekMessage to return false in the game loop
                return 0; // Handled
            }
            case WM_DESTROY:
            {
                AGK_INFO("WM_DESTROY received.");
                // Clean up the window's entry in our map, as the window is being destroyed
                s_WindowMap.erase(hWnd);
                return 0;
            }
            case WM_SIZE:
            {
                // Update window dimensions
                pWindow->m_Width = LOWORD(lParam);
                pWindow->m_Height = HIWORD(lParam);
                AGK_INFO("Window resized to {}x{}", pWindow->m_Width, pWindow->m_Height);
                // In a real engine, you'd trigger a renderer resize event here
                return 0;
            }
            // You'll add more message handling here later for input, etc.
            // case WM_KEYDOWN:
            // case WM_KEYUP:
            // case WM_MOUSEMOVE:
            // ...
            case WM_PAINT:
            {
                // For a pure Win32 app, you'd draw here. For a game engine,
                // our renderer will handle drawing outside the message loop.
                // Just validate the paint request to avoid continuous WM_PAINT messages.
                ValidateRect(hWnd, nullptr);
                return 0;
            }
            }
        }

        // Let the default window procedure handle any messages we don't explicitly process
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

} // namespace Angaraka