#ifndef THREADS_OF_KALIYUGA_GAME_HPP
#define THREADS_OF_KALIYUGA_GAME_HPP

#include <Angaraka/Base.hpp>
#include <Angaraka/AIBase.hpp>
#include <Angaraka/DialogueSystem.hpp>

namespace Angaraka {
    class DirectX12GraphicsSystem;

    namespace Core {
        class BundleManager;
        class CachedResourceManager;
    }

    namespace AI {
        class AIManager;
        class NPCManager;
        class DialogueSystem;
    }
}

namespace ThreadsOfKaliyuga
{
    class InputSystem;

    class Game
    {
    public:
        Game() = default;
        ~Game() = default;

        bool Initialize();
        void Run();
        void Shutdown();

    private:

    private:
        Angaraka::Reference<Angaraka::DirectX12GraphicsSystem> m_graphicsSystem{ nullptr };
        Angaraka::Reference<Angaraka::Core::CachedResourceManager> m_resourceManager{ nullptr };
        Angaraka::Reference<Angaraka::Core::BundleManager> m_bundleManager{ nullptr };
        Angaraka::Reference<InputSystem> m_inputSystem{ nullptr };

        // AI Integration Layer systems
        Angaraka::Reference<Angaraka::AI::AIManager> m_aiManager{ nullptr };
        Angaraka::Reference<Angaraka::AI::NPCManager> m_npcManager{ nullptr };
        Angaraka::Reference<Angaraka::AI::DialogueSystem> m_dialogueSystem{ nullptr };

        // --- Timing ---
        LARGE_INTEGER m_perfFreq;     // Performance Counter Frequency
        LARGE_INTEGER m_lastTime;     // Time at the end of the last frame
        Angaraka::F32 m_deltaTime{ 0.0f };    // Time elapsed since last frame (in seconds)

        // Game state
        bool m_isInDialogue{ false };
        Angaraka::String m_currentDialogueNPC;

        // Initialization helpers
        bool InitializeEngineCore();
        bool InitializeAISystems();
        bool InitializeGameSystems();

        // Update helpers  
        void UpdateAISystems(Angaraka::F32 deltaTime);
        void UpdateGameLogic(Angaraka::F32 deltaTime);
        void HandlePlayerInput();

        // Dialogue system integration
        void OnDialogueStarted(const Angaraka::String& npcId);
        void OnDialogueEnded(const Angaraka::String& npcId, Angaraka::AI::DialogueEndReason reason);
        void OnDialogueMessage(const Angaraka::String& npcId, const Angaraka::String& message, Angaraka::AI::DialogueTone tone);
        void OnChoiceSelected(const Angaraka::String& npcId, uint32_t choiceIndex);

        // Test/Demo functionality
        void SpawnTestNPCs();
        void HandleTestDialogueInput();
        void LogDialogueSystemStatus();

        // Shutdown helpers
        void ShutdownAISystems();
        void ShutdownEngineCore();

    };
}


#endif // THREADS_OF_KALIYUGA_GAME_HPP