#include "Game.hpp"
#include <objbase.h> // For CoInitializeEx and CoUninitialize
#include <Angaraka/Log.hpp>
#include <Angaraka/Asset/BundleManager.hpp>

// AI Integration Layer includes
#include <Angaraka/AIManager.hpp>
#include <Angaraka/NPCManager.hpp>
#include <Angaraka/DialogueSystem.hpp>

import Angaraka.Core.Config;
import Angaraka.Core.Events;
import Angaraka.Core.Window;
import Angaraka.Core.Resources;
import Angaraka.Core.ResourceCache;
import Angaraka.Input.Windows;
import Angaraka.Graphics.DirectX12;
import Angaraka.Graphics.DirectX12.Texture;
import Angaraka.Graphics.DirectX12.Mesh;
import Angaraka.Graphics.DirectX12.SceneManager;

import ThreadsOfKaliyuga.Input;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTK12.lib")
#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "onnxruntime.lib")
#pragma comment(lib, "onnxruntime_providers_cuda.lib")

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 616; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12"; }


// ==================================================================================
// Application Entry Point
// ==================================================================================

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // Initialize COM for WIC (Windows Imaging Component)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        AGK_FATAL("Failed to initialize COM for WIC. HRESULT: {:#x}", hr);
        return -1;
    }

    bool failure = false;
    ThreadsOfKaliyuga::Game game;

    if (game.Initialize()) {
        game.Run();
    }
    else {
        failure = true;
    }

    game.Shutdown();
    CoUninitialize();

    return failure ? -1 : 0;
}

// ==================================================================================
// Game Class Implementation
// ==================================================================================
namespace ThreadsOfKaliyuga {

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

#pragma region Game Class Implementation
    // ==================================================================================
    // Initialization
    // ==================================================================================

    bool Game::Initialize()
    {
        AGK_APP_INFO("Starting Threads of Kaliyuga with AI Integration...");

        // Initialize core engine systems
        if (!InitializeEngineCore()) {
            AGK_APP_FATAL("Failed to initialize engine core systems!");
            return false;
        }

        // Initialize AI systems
        if (!InitializeAISystems()) {
            AGK_APP_FATAL("Failed to initialize AI systems!");
            return false;
        }

        // Initialize game-specific systems
        if (!InitializeGameSystems()) {
            AGK_APP_FATAL("Failed to initialize game systems!");
            return false;
        }

        // Initialize scene manager
        if (!InitializeScene()) {
            AGK_APP_FATAL("Failed to initialize scene manager!");
            return false;
        }

        AGK_APP_INFO("Threads of Kaliyuga initialized successfully!");
        AGK_APP_INFO("Press 'T' to test dialogue system, 'ESC' to exit dialogue");

        return true;
    }

    bool Game::InitializeEngineCore()
    {
        // Initialize configuration
        Angaraka::Config::ConfigManager::Initialize();
        config = Angaraka::Config::ConfigManager::GetConfig();

        // Initialize logging
        Angaraka::Logger::Framework::Initialize();

        // Create window
        Angaraka::WindowCreateInfo windowInfo;
        windowInfo.Title = UTF8ToWString(config.window.title);
        windowInfo.Width = config.window.width;
        windowInfo.Height = config.window.height;
        windowInfo.Fullscreen = config.renderer.fullscreen;

        if (!window.Create(windowInfo)) {
            AGK_APP_FATAL("Failed to create main window!");
            return false;
        }

        // Initialize graphics system
        m_graphicsSystem = Angaraka::CreateReference<Angaraka::DirectX12GraphicsSystem>();
        if (!m_graphicsSystem->Initialize(window.GetHandle(), config)) {
            AGK_APP_FATAL("Failed to initialize graphics system");
            return false;
        }
        AGK_APP_INFO("GraphicsSystem initialized");

        // Initialize input system
        m_inputSystem = Angaraka::CreateReference<InputSystem>();
        if (!m_inputSystem->Initialize(window.GetHandle(), m_graphicsSystem->GetCamera(), config.window.fullscreen)) {
            AGK_APP_FATAL("Failed to initialize input system");
            return false;
        }
        m_inputSystem->SetupEvents();
        AGK_APP_INFO("InputSystem initialized");

        
        // Initialize resource manager
        auto cacheConfig = config.renderer.resourceCache.ToMemoryBudget();
        m_resourceManager = Angaraka::CreateReference<Angaraka::Core::CachedResourceManager>(
            config.assetsBasePath,
            cacheConfig
        );
        m_resourceManager->SetGraphicsFactory(m_graphicsSystem->GetGraphicsFactory());

        // Initialize bundle manager
        m_bundleManager = Angaraka::CreateReference<Angaraka::Core::BundleManager>(m_resourceManager.get(), m_graphicsSystem.get());
        m_bundleManager->SetGlobalProgressCallback(OnBundleProgress);

        std::filesystem::path bundlesPath = "Assets/bundles";
        if (!m_bundleManager->Initialize(bundlesPath)) {
            AGK_APP_WARN("No asset bundles found in {}", bundlesPath.string());
        }

        m_bundleManager->StartAsyncLoading();
        m_bundleManager->LoadAllAutoLoadBundles();
        m_bundleManager->LoadBundle("Environment_Forest");

        AGK_APP_INFO("Resource systems initialized");

        // Initialize high-resolution timer
        if (!QueryPerformanceFrequency(&m_perfFreq) || !QueryPerformanceCounter(&m_lastTime)) {
            AGK_APP_FATAL("Failed to initialize high-resolution timer");
            return false;
        }

        return true;
    }

    bool Game::InitializeAISystems()
    {
        AGK_APP_INFO("Initializing AI Integration Layer...");

        // Initialize AI Manager
        m_aiManager = Angaraka::CreateReference<Angaraka::AI::AIManager>();
        if (!m_aiManager->Initialize(m_resourceManager)) {
            AGK_APP_ERROR("Failed to initialize AIManager");
            return false;
        }
        AGK_APP_INFO("AIManager initialized");

        // Initialize NPC Manager
        m_npcManager = Angaraka::CreateReference<Angaraka::AI::NPCManager>(
            m_resourceManager,
            m_aiManager,
            m_graphicsSystem
        );

        Angaraka::AI::NPCManagerSettings npcSettings;
        npcSettings.maxActiveNPCs = 20;
        npcSettings.maxUpdateDistance = 100.0f;
        npcSettings.enableDebugLogging = true;

        if (!m_npcManager->Initialize(npcSettings)) {
            AGK_APP_ERROR("Failed to initialize NPCManager");
            return false;
        }
        AGK_APP_INFO("NPCManager initialized with {} default templates",
            m_npcManager->GetRegisteredTemplates().size());

        // Initialize Dialogue System
        m_dialogueSystem = Angaraka::CreateReference<Angaraka::AI::DialogueSystem>(
            m_resourceManager,
            m_aiManager,
            m_npcManager
        );

        Angaraka::AI::DialogueSystemSettings dialogueSettings;
        dialogueSettings.messageDisplaySpeed = 30.0f;  // Characters per second
        dialogueSettings.conversationTimeout = 45.0f;  // 45 second timeout
        dialogueSettings.enableTypingEffect = true;
        dialogueSettings.enableRelationshipFeedback = true;
        dialogueSettings.enableDebugInfo = true;

        if (!m_dialogueSystem->Initialize(dialogueSettings)) {
            AGK_APP_ERROR("Failed to initialize DialogueSystem");
            return false;
        }

        // Set up dialogue system callbacks
        m_dialogueSystem->SetConversationStartCallback(
            [this](const Angaraka::String& npcId) { OnDialogueStarted(npcId); }
        );

        m_dialogueSystem->SetConversationEndCallback(
            [this](const Angaraka::String& npcId, Angaraka::AI::DialogueEndReason reason) { OnDialogueEnded(npcId, reason); }
        );

        m_dialogueSystem->SetMessageDisplayCallback(
            [this](const Angaraka::String& npcId, const Angaraka::String& message, Angaraka::AI::DialogueTone tone) {
                OnDialogueMessage(npcId, message, tone);
            }
        );

        m_dialogueSystem->SetChoiceSelectedCallback(
            [this](const Angaraka::String& npcId, uint32_t choiceIndex) { OnChoiceSelected(npcId, choiceIndex); }
        );

        AGK_APP_INFO("DialogueSystem initialized with callbacks");

        // Spawn test NPCs for demonstration
        SpawnTestNPCs();

        // load faction dialog models
        if (!m_aiManager->LoadSharedDialogueModel("Assets/ai/dialogue.onnx")) {
            AGK_APP_ERROR("Failed to load shared dialogue model");
        }
        if (!m_aiManager->LoadFactionConfigs("Assets/ai")) {
            AGK_APP_ERROR("Failed to load faction configurations");
        }
        if (!m_aiManager->LoadSharedTokenizer("Assets/ai")) {
            AGK_APP_ERROR("Failed to load shared tokenizer");
        }

        return true;
    }

    bool Game::InitializeGameSystems()
    {
        // Initialize any additional game-specific systems here
        // For now, this is a placeholder for future game systems

        AGK_APP_INFO("Game systems initialized");
        return true;
    }

#pragma endregion

#pragma region Game Loop

    // ==================================================================================
    // Main Game Loop
    // ==================================================================================

    void Game::Run()
    {
        bool running = true;
        AGK_APP_INFO("Entering main game loop...");

        while (running)
        {
            // Process Windows messages
            if (!window.ProcessMessages()) {
                running = false; // WM_QUIT was received
            }

            // Calculate delta time
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            m_deltaTime = static_cast<Angaraka::F32>(currentTime.QuadPart - m_lastTime.QuadPart) / static_cast<Angaraka::F32>(m_perfFreq.QuadPart);
            m_lastTime = currentTime;

            Update();
            Render();

            // Periodic cache monitoring
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

    void Game::Update() {

        // Handle player input
        HandlePlayerInput();

        // Update engine systems
        m_inputSystem->Update(m_deltaTime);

        // Update AI systems
        UpdateAISystems(m_deltaTime);

        // Update game logic
        UpdateGameLogic(m_deltaTime);

        // Update scene management
        UpdateScene(m_deltaTime);
    }

    void Game::Render() {

        // Render frame
        m_graphicsSystem->BeginFrame(m_deltaTime);

        // Render game objects
        Angaraka::Core::Resource* uvGridTexture = m_resourceManager->GetResource<Angaraka::Core::Resource>("character/player_diffuse").get();
        if (uvGridTexture) {
            m_graphicsSystem->RenderTexture(uvGridTexture);
        }

        RenderScene();

        m_graphicsSystem->EndFrame();
        m_graphicsSystem->Present();
    }

    void Game::UpdateAISystems(Angaraka::F32 deltaTime)
    {
        // Update AI Manager
        if (m_aiManager) {
            m_aiManager->Update(deltaTime);
        }

        // Update NPC Manager
        if (m_npcManager) {
            m_npcManager->Update(deltaTime);

            // Update player position for NPCs (this would come from player controller in real game)
            Vector3 playerPos = Vector3(0.0f, 0.0f, 0.0f); // Placeholder - get from actual player
            m_npcManager->UpdatePlayerLocation(playerPos);
        }

        // Update Dialogue System
        if (m_dialogueSystem) {
            m_dialogueSystem->Update(deltaTime);
        }

        // Log system status periodically
        static Angaraka::F32 statusLogTimer = 0.0f;
        statusLogTimer += deltaTime;
        if (statusLogTimer >= 10.0f) { // Every 10 seconds
            statusLogTimer = 0.0f;
            LogDialogueSystemStatus();
        }
    }

    void Game::UpdateGameLogic(Angaraka::F32 deltaTime)
    {
        // Handle test dialogue input
        HandleTestDialogueInput();

        // Update other game logic here
        // This is where you'd update player movement, game state, etc.
    }

    void Game::HandlePlayerInput()
    {
        // Input is handled by InputSystem, but we can add game-specific input handling here
        // For now, dialogue system input is handled in HandleTestDialogueInput()
    }

#pragma endregion


    // ==================================================================================
    // Scene Management
    // ==================================================================================
    bool Game::InitializeScene()
    {
        AGK_INFO("Initializing scene management...");

        // Create scene manager with dependencies
        m_sceneManager = Angaraka::CreateScope<Angaraka::Graphics::DirectX12::Scene::Manager>(
            m_resourceManager.get(),
            m_graphicsSystem.get()
        );

        AGK_INFO("Scene initialized with {} objects", m_sceneManager->GetObjectCount());
        return true;
    }


    void Game::UpdateScene(Angaraka::F32 deltaTime)
    {
        if (m_sceneManager)
        {
            // Load test scene
            m_sceneManager->CreateTestScene();
            m_sceneManager->Update(deltaTime);
        }
    }

    void Game::RenderScene()
    {
        if (!m_sceneManager)
            return;

        // Get all visible objects
        const auto& objects = m_sceneManager->GetVisibleObjects();

        for (const auto& object : objects)
        {
            if (!object.visible)
                continue;

            // Get mesh resource
            auto meshResource = m_resourceManager->GetResource<Angaraka::Core::Resource>(object.meshResourceId);
            if (!meshResource)
            {
                AGK_WARN("Mesh resource '{}' not found for object '{}'", object.meshResourceId, object.name);
                continue;
            }

            // Calculate world matrix for this object using Angaraka Math
            auto modelMatrix = object.GetWorldMatrix();

            // Render the mesh with the world matrix
            m_graphicsSystem->RenderMesh(meshResource.get(), modelMatrix);
        }
    }


    // ==================================================================================
    // Dialogue System Integration
    // ==================================================================================

    void Game::OnDialogueStarted(const Angaraka::String& npcId)
    {
        m_isInDialogue = true;
        m_currentDialogueNPC = npcId;

        AGK_APP_INFO("=== DIALOGUE STARTED ===");
        AGK_APP_INFO("Talking to NPC: {}", npcId);
        AGK_APP_INFO("Press number keys (1-4) to select dialogue choices");
        AGK_APP_INFO("Press ESC to end conversation");
    }

    void Game::OnDialogueEnded(const Angaraka::String& npcId, Angaraka::AI::DialogueEndReason reason)
    {
        m_isInDialogue = false;
        m_currentDialogueNPC = "";

        Angaraka::String reasonText = reason == Angaraka::AI::DialogueEndReason::PlayerChoice ? "player choice" :
            reason == Angaraka::AI::DialogueEndReason::NPCDecision ? "NPC decision" :
            reason == Angaraka::AI::DialogueEndReason::Timeout ? "timeout" :
            reason == Angaraka::AI::DialogueEndReason::Interrupted ? "interrupted" : "error";

        AGK_APP_INFO("=== DIALOGUE ENDED ===");
        AGK_APP_INFO("Conversation with {} ended due to: {}", npcId, reasonText);
        AGK_APP_INFO("Press 'T' to start a new conversation");
    }

    void Game::OnDialogueMessage(const Angaraka::String& npcId, const Angaraka::String& message, Angaraka::AI::DialogueTone tone)
    {
        Angaraka::String toneText = tone == Angaraka::AI::DialogueTone::Friendly ? "Friendly" :
            tone == Angaraka::AI::DialogueTone::Hostile ? "Hostile" :
            tone == Angaraka::AI::DialogueTone::Respectful ? "Respectful" :
            tone == Angaraka::AI::DialogueTone::Philosophical ? "Philosophical" : "Neutral";

        AGK_APP_INFO("[{}] ({}): {}", npcId, toneText, message);

        // Display current choices
        auto choices = m_dialogueSystem->GetCurrentChoices();
        if (!choices.empty()) {
            AGK_APP_INFO("--- Your Response Options ---");
            for (size_t i = 0; i < choices.size(); ++i) {
                Angaraka::String availabilityText = choices[i].isAvailable ? "" : " (UNAVAILABLE)";
                AGK_APP_INFO("[{}] {}{}", i + 1, choices[i].choiceText, availabilityText);
            }
        }
    }

    void Game::OnChoiceSelected(const Angaraka::String& npcId, uint32_t choiceIndex)
    {
        auto choices = m_dialogueSystem->GetCurrentChoices();
        if (choiceIndex < choices.size()) {
            AGK_APP_INFO("Player selected: {}", choices[choiceIndex].choiceText);
        }
    }

    // ==================================================================================
    // Test/Demo Functionality
    // ==================================================================================

    void Game::SpawnTestNPCs()
    {
        AGK_APP_INFO("Spawning test NPCs for dialogue demonstration...");

        // Spawn NPCs from different factions
        Vector3 ashvatthaPos(5.0f, 0.0f, 0.0f);
        Vector3 vaikunthaPos(-5.0f, 0.0f, 0.0f);
        Vector3 yugaPos(0.0f, 0.0f, 5.0f);

        if (m_npcManager->SpawnNPC("test_sage", "scholar_ashvattha", ashvatthaPos)) {
            AGK_APP_INFO("Spawned Ashvattha sage at position (5, 0, 0)");
        }

        if (m_npcManager->SpawnNPC("test_officer", "guard_vaikuntha", vaikunthaPos)) {
            AGK_APP_INFO("Spawned Vaikuntha officer at position (-5, 0, 0)");
        }

        if (m_npcManager->SpawnNPC("test_rebel", "civilian_yuga_striders", yugaPos)) {
            AGK_APP_INFO("Spawned Yuga Strider at position (0, 0, 5)");
        }

        AGK_APP_INFO("Test NPCs spawned successfully!");
    }

    void Game::HandleTestDialogueInput()
    {
        // Note: In a real implementation, you'd integrate this with your InputSystem
        // For now, we'll use basic Windows input for testing

        if (GetAsyncKeyState('T') & 0x8000) {
            static bool tKeyWasPressed = false;
            if (!tKeyWasPressed && !m_isInDialogue) {
                tKeyWasPressed = true;

                // Start dialogue with a random test NPC
                std::vector<Angaraka::String> testNPCs = { "test_sage", "test_officer", "test_rebel" };
                static int currentNPC = 0;
                Angaraka::String npcToTalk = testNPCs[currentNPC % testNPCs.size()];
                currentNPC++;

                AGK_APP_INFO("Attempting to start dialogue with {}...", npcToTalk);
                m_dialogueSystem->StartConversation(npcToTalk, "greeting");
            }

            if (!(GetAsyncKeyState('T') & 0x8000)) {
                tKeyWasPressed = false;
            }
        }

        // Handle dialogue choice selection
        if (m_isInDialogue) {
            for (int i = 1; i <= 4; ++i) {
                if (GetAsyncKeyState('0' + i) & 0x8000) {
                    static bool numberKeyWasPressed[5] = { false };
                    if (!numberKeyWasPressed[i]) {
                        numberKeyWasPressed[i] = true;

                        if (m_dialogueSystem->CanMakeChoice()) {
                            auto choices = m_dialogueSystem->GetCurrentChoices();
                            if ((i - 1) < static_cast<int>(choices.size())) {
                                m_dialogueSystem->SelectChoice(i - 1);
                            }
                        }
                    }

                    if (!(GetAsyncKeyState('0' + i) & 0x8000)) {
                        numberKeyWasPressed[i] = false;
                    }
                }
            }

            // ESC to end dialogue
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                static bool escKeyWasPressed = false;
                if (!escKeyWasPressed) {
                    escKeyWasPressed = true;
                    m_dialogueSystem->EndConversation(Angaraka::AI::DialogueEndReason::PlayerChoice);
                }

                if (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
                    escKeyWasPressed = false;
                }
            }
        }
    }

    void Game::LogDialogueSystemStatus()
    {
        if (m_dialogueSystem) {
            AGK_APP_INFO("=== AI System Status ===");
            AGK_APP_INFO("Active NPCs: {}", m_npcManager->GetActiveNPCCount());
            AGK_APP_INFO("In Dialogue: {}", m_isInDialogue ? "Yes" : "No");
            AGK_APP_INFO("Average AI Response Time: {:.1f}ms", m_dialogueSystem->GetAverageAIResponseTime());

            if (m_isInDialogue) {
                AGK_APP_INFO("Current conversation with: {}", m_currentDialogueNPC);
            }
        }
    }

    // ==================================================================================
    // Shutdown
    // ==================================================================================

    void Game::Shutdown()
    {
        AGK_APP_INFO("Shutting down Threads of Kaliyuga...");

        // Shutdown AI systems first
        ShutdownAISystems();

        // Shutdown core engine systems
        ShutdownEngineCore();

        AGK_APP_INFO("Threads of Kaliyuga shutdown complete");
    }

    void Game::ShutdownAISystems()
    {
        for (long i{ 0 }; i < m_dialogueSystem.use_count(); ++i) {
            m_dialogueSystem.reset();
        }
        m_dialogueSystem = nullptr;
        AGK_APP_INFO("DialogueSystem shutdown");

        for (long i{ 0 }; i < m_npcManager.use_count(); ++i) {
            m_npcManager.reset();
        }
        m_npcManager = nullptr;
        AGK_APP_INFO("NPCManager shutdown");

        for (long i{ 0 }; i < m_aiManager.use_count(); ++i) {
            m_aiManager.reset();
        }
        m_aiManager = nullptr;
        AGK_APP_INFO("AIManager shutdown");
    }

    void Game::ShutdownEngineCore()
    {
        window.Destroy();

        for (long i{ 0 }; i < m_inputSystem.use_count(); ++i) {
            m_inputSystem.reset();
        }
        m_inputSystem = nullptr;
        AGK_APP_INFO("InputSystem shutdown");

        for (long i{ 0 }; i < m_bundleManager.use_count(); ++i) {
            m_bundleManager.reset();
        }
        m_bundleManager = nullptr;
        AGK_APP_INFO("BundleManager shutdown");

        for (long i{ 0 }; i < m_resourceManager.use_count(); ++i) {
            m_resourceManager.reset();
        }
        m_resourceManager = nullptr;
        AGK_APP_INFO("ResourceManager shutdown");

        for (long i{ 0 }; i < m_graphicsSystem.use_count(); ++i) {
            if (i == 0) {
                m_graphicsSystem->DisplayStats();
            }
            m_graphicsSystem.reset();
        }
        m_graphicsSystem = nullptr;
        AGK_APP_INFO("GraphicsSystem shutdown");

        Angaraka::Logger::Framework::Shutdown();
    }
}