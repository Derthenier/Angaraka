#include "Game.hpp"
#include <Angaraka/Log.hpp>
#include <objbase.h> // For CoInitializeEx and CoUninitialize

import Angaraka.Core.Config;
import Angaraka.Core.Window;
import Angaraka.Core.Resources;
import Angaraka.Graphics.DirectX12;
import Angaraka.Input.Windows;

import ThreadsOfKaliyuga.Input;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTK12.lib")
#pragma comment(lib, "DirectXTex.lib")

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 616; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12"; }

namespace ThreadsOfKaliyuga
{
    namespace {
        Angaraka::Window window;
        Angaraka::Config::EngineConfig config;

        std::string WStringToUTF8(const std::wstring& wstr) {
            if (wstr.empty()) return {};

            int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (size <= 0) return {};

            std::string result(size - 1, 0); // omit null terminator
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
            return result;
        }

        std::wstring UTF8ToWString(const std::string& str) {
            if (str.empty()) return {};

            int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
            if (size <= 0) return {};

            std::wstring wstr(size - 1, 0); // exclude null terminator
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
            return wstr;
        }
    }

    bool Game::Initialize()
    {
        // Initialize game components here
        Angaraka::Config::ConfigManager::Initialize();
        config = Angaraka::Config::ConfigManager::GetConfig();

        Angaraka::Logger::Framework::Initialize();

        AGK_APP_INFO("Starting Threads of Kaliyuga...");

        Angaraka::WindowCreateInfo windowInfo;
        windowInfo.Title = UTF8ToWString(config.window.title);
        windowInfo.Width = config.window.width;
        windowInfo.Height = config.window.height;
        windowInfo.Fullscreen = config.renderer.fullscreen;

        if (!window.Create(windowInfo))
        {
            AGK_APP_FATAL("Failed to create main window!");
            return false; // Indicate error
        }

        m_graphicsSystem = new Angaraka::DirectX12GraphicsSystem();
        if (!m_graphicsSystem->Initialize(window.GetHandle(), windowInfo.Width, windowInfo.Height, config.renderer.debugLayerEnabled)) {
            AGK_APP_FATAL("Failed to initialize graphics system. Stopping engine...");
            return false;
        }

        AGK_APP_INFO("GraphicsSystem initialized.");

        m_resourceManager = new Angaraka::Core::ResourceManager(Angaraka::Events::EventManager::Get());
        
        m_inputSystem = new InputSystem();
        if (!m_inputSystem->Initialize(window.GetHandle(), m_graphicsSystem->GetCamera(), config.window.fullscreen)) {
            AGK_APP_FATAL("Failed to initialize input system. Stopping engine...");
            return false;
        }

        m_inputSystem->SetupEvents(); // Setup input events

        AGK_APP_INFO("InputManager initialized.");

        // --- Initialize High-Resolution Timer ---
        if (!QueryPerformanceFrequency(&m_perfFreq)) {
            AGK_APP_INFO("QueryPerformanceFrequency failed!");
            return false;
        }
        if (!QueryPerformanceCounter(&m_lastTime)) {
            AGK_APP_INFO("QueryPerformanceCounter failed for initial time!");
            return false;
        }



        return true;
    }

    void Game::Run() {
        bool running = true;
        while (running)
        {
            // Process Windows messages
            if (!window.ProcessMessages())
            {
                running = false; // WM_QUIT was received
            }

            // --- Calculate Delta Time ---
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);

            m_deltaTime = static_cast<float>(currentTime.QuadPart - m_lastTime.QuadPart) / static_cast<float>(m_perfFreq.QuadPart);
            m_lastTime = currentTime; // Update for the next frame

            m_inputSystem->Update(m_deltaTime); // Update input system

            m_graphicsSystem->BeginFrame(m_deltaTime, config.renderer.clearRed, config.renderer.clearGreen, config.renderer.clearBlue, 0.7f);
            m_graphicsSystem->EndFrame();
        }
    }

    void Game::Shutdown()
    {
        AGK_APP_INFO("Shutting down Threads of Kaliyuga...");

        if (m_inputSystem) {
            m_inputSystem->Shutdown();
            delete m_inputSystem;
            m_inputSystem = nullptr;
        }

        if (m_resourceManager) {
            m_resourceManager->UnloadAllResources();
            delete m_resourceManager;
            m_resourceManager = nullptr;
        }

        if (m_graphicsSystem) {
            m_graphicsSystem->Shutdown();
            delete m_graphicsSystem;
            m_graphicsSystem = nullptr;
        }

        // Clean up game components here
        Angaraka::Logger::Framework::Shutdown();
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // Initialize COM for WIC (Windows Imaging Component)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        AGK_FATAL("Failed to initialize COM for WIC. HRESULT: {:#x}", hr);
        return -1;
    }

    ThreadsOfKaliyuga::Game game;
    if (game.Initialize())
    {
        game.Run();
    }
    else
    {
        return -1; // Initialization failed
    }

    game.Shutdown();
    CoUninitialize(); // Uninitialize COM

    return 0; // Exit successfully
}