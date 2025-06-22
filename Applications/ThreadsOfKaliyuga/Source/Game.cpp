#include "Game.hpp"
#include <objbase.h> // For CoInitializeEx and CoUninitialize
#include <Angaraka/Log.hpp>
#include <Angaraka/AssetBundleManager.hpp>

import Angaraka.Core.Config;
import Angaraka.Core.Window;
import Angaraka.Core.Resources;
import Angaraka.Core.ResourceCache;
import Angaraka.Input.Windows;
import Angaraka.Graphics.DirectX12;
import Angaraka.Graphics.DirectX12.Texture;


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

        Angaraka::String WStringToUTF8(const std::wstring& wstr) {
            if (wstr.empty()) return {};

            int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (size <= 0) return {};

            Angaraka::String result(size - 1, 0); // omit null terminator
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
            return result;
        }

        std::wstring UTF8ToWString(const Angaraka::String& str) {
            if (str.empty()) return {};

            int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
            if (size <= 0) return {};

            std::wstring wstr(size - 1, 0); // exclude null terminator
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
            return wstr;
        }

        void OnBundleProgress(const Angaraka::Core::BundleLoadProgress& progress) {
            AGK_APP_INFO("Bundle '{}' loading: {:.1f}% ({}/{})",
                progress.bundleName, progress.progress * 100.0f,
                progress.assetsLoaded, progress.totalAssets);

            if (progress.state == Angaraka::Core::BundleLoadState::Loaded) {
                AGK_APP_INFO("Bundle '{}' loaded successfully!", progress.bundleName);
            }
            else if (progress.state == Angaraka::Core::BundleLoadState::Failed) {
                AGK_APP_ERROR("Bundle '{}' failed to load: {}",
                    progress.bundleName, progress.errorMessage);
            }
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
        if (!m_graphicsSystem->Initialize(window.GetHandle(), config)) {
            AGK_APP_FATAL("Failed to initialize graphics system. Stopping engine...");
            return false;
        }

        AGK_APP_INFO("GraphicsSystem initialized.");
        
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

        // Get cache config from engine config
        auto cacheConfig = config.renderer.resourceCache.ToMemoryBudget();

        m_resourceManager = new Angaraka::Core::CachedResourceManager(
            config.assetsBasePath,
            Angaraka::Events::EventManager::Get(),
            cacheConfig
        );
        m_resourceManager->SetGraphicsFactory(m_graphicsSystem->GetGraphicsFactory());

        // Initialize bundle manager
        m_bundleManager = new Angaraka::Core::BundleManager(m_resourceManager, m_graphicsSystem);

        // Set global progress callback
        m_bundleManager->SetGlobalProgressCallback(OnBundleProgress);

        // Initialize with bundles directory
        std::filesystem::path bundlesPath = "Assets/bundles";
        if (!m_bundleManager->Initialize(bundlesPath)) {
            AGK_APP_WARN("No asset bundles found in {}", bundlesPath.string());
        }

        // Start async loading
        m_bundleManager->StartAsyncLoading();

        // Load all auto-load bundles
        m_bundleManager->LoadAllAutoLoadBundles();

        m_bundleManager->LoadBundle("Environment_Forest");

        AGK_APP_INFO("BundleManager initialized and loading started.");

        m_resourceManager->LogCacheStatus();

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

            m_deltaTime = static_cast<Angaraka::F32>(currentTime.QuadPart - m_lastTime.QuadPart) / static_cast<Angaraka::F32>(m_perfFreq.QuadPart);
            m_lastTime = currentTime; // Update for the next frame

            m_inputSystem->Update(m_deltaTime); // Update input system

            m_graphicsSystem->BeginFrame(m_deltaTime);

            Angaraka::Core::Resource* uvGridTexture = m_resourceManager->GetResource<Angaraka::Core::Resource>("character/player_diffuse").get();
            m_graphicsSystem->RenderTexture(uvGridTexture);
            Angaraka::Core::Resource* player = m_resourceManager->GetResource<Angaraka::Core::Resource>("character/player_mesh").get();
            m_graphicsSystem->RenderMesh(player);
            Angaraka::Core::Resource* tree = m_resourceManager->GetResource<Angaraka::Core::Resource>("environment/tree_oak").get();
            m_graphicsSystem->RenderMesh(tree);

            m_graphicsSystem->EndFrame();
            m_graphicsSystem->Present();


            // Periodic cache monitoring (every 5 seconds)
            static Angaraka::F32 cacheMonitorTimer = 0.0f;
            cacheMonitorTimer += m_deltaTime;
            if (cacheMonitorTimer >= 5.0f) {
                if (!m_resourceManager->IsCacheHealthy()) {
                    AGK_APP_WARN("Resource cache health degraded - {}% utilization",
                        static_cast<int>(m_resourceManager->GetCacheUtilization() * 100));
                }
                cacheMonitorTimer = 0.0f;
            }
        }
    }

    void Game::Shutdown()
    {
        AGK_APP_INFO("Shutting down Threads of Kaliyuga...");

        window.Destroy();

        if (m_inputSystem) {
            m_inputSystem->Shutdown();
            delete m_inputSystem;
            m_inputSystem = nullptr;
        }

        m_graphicsSystem->DisplayStats();

        // Shutdown bundle manager first (stops async loading)
        if (m_bundleManager) {
            m_bundleManager->Shutdown();
            delete m_bundleManager;
            m_bundleManager = nullptr;
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

    bool failure{ false };
    ThreadsOfKaliyuga::Game game;
    if (game.Initialize())
    {
        game.Run();
    }
    else
    {
        failure = true;
    }

    game.Shutdown();
    CoUninitialize(); // Uninitialize COM

    return failure ? -1 : 0; // Exit successfully
}