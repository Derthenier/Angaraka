#pragma once

#include <Angaraka/AIBase.hpp>
#include "Angaraka/NPCController.hpp"

// Forward declarations for integration
namespace Angaraka::Core {
    class EventManager;
    class CachedResourceManager;
}

namespace Angaraka::AI {
    class AIManager;
    class NPCManager;
}

import Angaraka.Core.Events;

namespace Angaraka::AI {

    // Dialogue UI integration events
    class DialogueStartedEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Dialogue)
        EVENT_CLASS_TYPE(DialogueStartedEvent)

        String npcId;
        String npcName;
        NPCFaction npcFaction;
        Math::Vector3 npcPosition;
        String initialMessage;
        std::vector<String> playerOptions;
    };

    struct DialogueMessageEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Dialogue)
            EVENT_CLASS_TYPE(DialogueMessageEvent)

        String npcId;
        String message;
        String emotionalTone;
        F32 deliverySpeed{ 1.0f };        // Text scroll speed multiplier
        bool isPlayerMessage{ false };
        std::vector<String> playerOptions;
        bool showContinuePrompt{ true };
    };

    struct DialogueEndedEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Dialogue)
        EVENT_CLASS_TYPE(DialogueEndedEvent)

        String npcId;
        String reason;                     // "player_choice", "npc_decision", "timeout", "interrupted"
        F32 conversationDuration;
        U32 exchangeCount;
        F32 relationshipChange;
    };

    struct DialogueChoiceSelectedEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Dialogue)
        EVENT_CLASS_TYPE(DialogueChoiceSelectedEvent)

        String npcId;
        String playerChoice;
        U32 choiceIndex;
        String choiceContext;
    };

    // Dialogue state and options
    enum class DialogueState : U32 {
        Inactive = 0,
        Starting,
        WaitingForNPC,
        WaitingForPlayer,
        DisplayingMessage,
        WaitingForChoice,
        Processing,
        Ending,
        Count
    };

    enum class DialogueTone : U32 {
        Neutral = 0,
        Friendly,
        Hostile,
        Respectful,
        Dismissive,
        Philosophical,
        Urgent,
        Secretive,
        Count
    };

    enum class DialogueEndReason : U32 {
        PlayerChoice = 0,
        NPCDecision,
        Timeout,
        Interrupted,
        Error,
        Count
    };

    // Player dialogue choice with AI context
    struct DialogueChoice {
        String choiceText;                 // What the player says
        String choiceType;                 // "aggressive", "diplomatic", "questioning", etc.
        F32 relationshipImpact{ 0.0f };    // Predicted relationship change
        String expectedResponse;           // Predicted NPC reaction type
        bool requiresCheck{ false };       // Requires skill/reputation check
        String checkType;                  // "persuasion", "intimidation", "knowledge"
        F32 checkDifficulty{ 0.5f };       // 0.0 = easy, 1.0 = very hard
        bool isAvailable{ true };          // Can player select this option
        String unavailableReason;          // Why option is disabled
    };

    // Complete dialogue exchange data
    struct DialogueExchange {
        std::chrono::steady_clock::time_point timestamp;
        String speaker;                    // "player" or npcId
        String message;
        String emotionalTone;
        F32 relationshipImpact{ 0.0f };
        std::vector<String> topics;       // Topics discussed in this exchange
        String playerChoiceType;           // Type of player's choice that led to this
    };

    // Ongoing conversation state
    struct ActiveDialogue {
        String npcId;
        String npcName;
        NPCFaction npcFaction;
        DialogueState currentState{ DialogueState::Inactive };

        // Conversation flow
        std::vector<DialogueExchange> exchanges;
        std::vector<DialogueChoice> currentChoices;
        String currentTopic;
        String conversationContext;

        // Timing and control
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastExchangeTime;
        F32 timeoutDuration{ 30.0f };      // Conversation timeout in seconds
        bool isPaused{ false };

        // AI state
        bool waitingForAIResponse{ false };
        String pendingPlayerChoice;
        String pendingChoiceContext;

        // Relationship tracking
        F32 initialTrust{ 0.5f };
        F32 initialRespect{ 0.5f };
        F32 relationshipChangeThisConversation{ 0.0f };

        // Performance tracking
        U32 aiRequestCount{ 0 };
        F32 totalAIResponseTime{ 0.0f };
        F32 averageResponseTime{ 0.0f };
    };

    // Dialogue system settings
    struct DialogueSystemSettings {
        F32 messageDisplaySpeed{ 50.0f };          // Characters per second
        F32 choiceDisplayDelay{ 1.0f };             // Delay before showing choices
        F32 conversationTimeout{ 60.0f };          // Auto-end conversations
        F32 aiResponseTimeout{ 10.0f };            // Max wait for AI response
        U32 maxExchangesPerConversation{ 20 }; // Limit conversation length
        U32 maxPlayerChoices{ 4 };             // Max choices to show player
        bool enableTypingEffect{ true };           // Gradual text reveal
        bool enableVoiceActing{ false };           // Future: audio integration
        bool enableAutoSubtitles{ true };          // Display all dialogue as text
        bool enableRelationshipFeedback{ true };   // Show relationship changes
        bool enableDebugInfo{ false };             // Show AI timing/debug info
        F32 relationshipDisplayThreshold{ 0.1f };  // Min change to show feedback
    };

    // AI-driven dialogue conversation system
    class DialogueSystem {
    public:
        explicit DialogueSystem(
            Reference<Angaraka::Core::CachedResourceManager> resourceManager,
            Reference<Angaraka::AI::AIManager> aiManager,
            Reference<Angaraka::AI::NPCManager> npcManager
        );

        ~DialogueSystem();

        // Core lifecycle
        bool Initialize(const DialogueSystemSettings& settings = DialogueSystemSettings{});
        void Update(F32 deltaTime);
        void Shutdown();

        // Conversation management
        bool StartConversation(const String& npcId, const String& initialTopic = "");
        void EndConversation(DialogueEndReason reason = DialogueEndReason::PlayerChoice);
        void PauseConversation();
        void ResumeConversation();

        // Player interaction
        void SelectChoice(U32 choiceIndex);
        void SelectChoice(const String& choiceText);
        std::vector<DialogueChoice> GetCurrentChoices() const;
        bool CanMakeChoice() const;

        // Conversation state
        bool IsInConversation() const;
        const ActiveDialogue* GetActiveDialogue() const;
        DialogueState GetCurrentState() const { return m_activeDialogue ? m_activeDialogue->currentState : DialogueState::Inactive; }
        String GetCurrentNPCId() const { return m_activeDialogue ? m_activeDialogue->npcId : ""; }

        // AI integration
        void ProcessPlayerChoice(const String& choice, const String& choiceType);
        void HandleAIResponse(const String& npcId, const String& response, const String& emotionalTone);
        void RequestAIDialogue(const String& playerMessage, const String& context);

        // Conversation history and analysis
        std::vector<DialogueExchange> GetConversationHistory(const String& npcId) const;
        F32 GetRelationshipChange(const String& npcId) const;
        String GetConversationSummary(const String& npcId) const;
        bool SaveConversationHistory();
        bool LoadConversationHistory();

        // Choice generation and validation
        std::vector<DialogueChoice> GeneratePlayerChoices(const String& npcResponse, const String& context);
        void ValidateChoiceAvailability(DialogueChoice& choice, const String& npcId);
        F32 PredictChoiceOutcome(const DialogueChoice& choice, const String& npcId);

        // Settings and configuration
        void UpdateSettings(const DialogueSystemSettings& newSettings);
        const DialogueSystemSettings& GetSettings() const { return m_settings; }

        // UI integration helpers
        String GetFormattedMessage(const DialogueExchange& exchange) const;
        String GetRelationshipStatusText(const String& npcId) const;
        String GetChoicePreviewText(const DialogueChoice& choice) const;

        // Debug and diagnostics
        String GetDebugInfo() const;
        void LogConversationMetrics() const;
        F32 GetAverageAIResponseTime() const;

        // Event callbacks for extensibility
        using ConversationStartCallback = std::function<void(const String&)>;
        using ConversationEndCallback = std::function<void(const String&, DialogueEndReason)>;
        using MessageDisplayCallback = std::function<void(const String&, const String&, DialogueTone)>;
        using ChoiceSelectedCallback = std::function<void(const String&, U32)>;

        void SetConversationStartCallback(ConversationStartCallback callback) { m_startCallback = callback; }
        void SetConversationEndCallback(ConversationEndCallback callback) { m_endCallback = callback; }
        void SetMessageDisplayCallback(MessageDisplayCallback callback) { m_messageCallback = callback; }
        void SetChoiceSelectedCallback(ChoiceSelectedCallback callback) { m_choiceCallback = callback; }

    private:
        // System integration
        Reference<Angaraka::Core::CachedResourceManager> m_resourceManager;
        Reference<Angaraka::AI::AIManager> m_aiManager;
        Reference<Angaraka::AI::NPCManager> m_npcManager;

        // Dialogue state
        std::unique_ptr<ActiveDialogue> m_activeDialogue;
        std::unordered_map<String, std::vector<DialogueExchange>> m_conversationHistory;
        DialogueSystemSettings m_settings;
        bool m_isInitialized{ false };

        // Choice generation and caching
        std::unordered_map<String, std::vector<DialogueChoice>> m_choiceCache;
        std::unordered_map<String, F32> m_relationshipCache;

        // Performance tracking
        std::chrono::steady_clock::time_point m_lastUpdate;
        U32 m_totalConversations{ 0 };
        F32 m_totalConversationTime{ 0.0f };
        U32 m_totalExchanges{ 0 };

        // Event callbacks
        ConversationStartCallback m_startCallback;
        ConversationEndCallback m_endCallback;
        MessageDisplayCallback m_messageCallback;
        ChoiceSelectedCallback m_choiceCallback;

        // Event handling
        void SubscribeToEvents();
        void UnsubscribeFromEvents();
        void OnNPCDialogueResponse(const NPCDialogueResponseEvent& response);
        void OnNPCStateChange(const NPCStateChangeEvent& stateChange);
        void OnPlayerInteraction(const NPCInteractionEvent& interaction);

        // Internal conversation flow
        void ProcessConversationState(F32 deltaTime);
        void TransitionToState(DialogueState newState);
        void HandleStateTimeout();

        // AI request management
        void SendAIDialogueRequest(const String& playerMessage);
        void ProcessAIResponse(const String& response, const String& emotionalTone);
        String BuildConversationContext() const;
        String FormatConversationHistory() const;

        // Choice generation
        std::vector<DialogueChoice> CreateDefaultChoices(const String& context);
        std::vector<DialogueChoice> CreateContextualChoices(const String& npcResponse, const String& npcId);
        void PopulateChoiceMetadata(DialogueChoice& choice, const String& npcId);
        DialogueTone ParseEmotionalTone(const String& toneString) const;

        // Relationship management
        void UpdateRelationshipFromChoice(const String& npcId, const DialogueChoice& choice);
        void TrackRelationshipChange(const String& npcId, F32 change);
        F32 CalculateRelationshipImpact(const String& choiceType, const String& npcId) const;

        // Conversation history management
        void AddExchange(const String& speaker, const String& message, const String& tone = "neutral");
        void UpdateConversationMetrics();
        void CleanupOldConversations();

        // Validation and error handling
        bool ValidateNPCForConversation(const String& npcId) const;
        bool ValidateChoiceIndex(U32 choiceIndex) const;
        void HandleConversationError(const String& error);
        void LogConversationError(const String& operation, const String& error) const;

        // Performance optimization
        void OptimizeChoiceGeneration();
        void CacheFrequentChoices();
        void PurgeOldCaches();

        // Utility methods
        String GetTimeStampString() const;
        String SanitizeMessage(const String& message) const;
        F32 CalculateTypingDuration(const String& message) const;
        DialogueTone GetDefaultToneForFaction(NPCFaction faction) const;
    };

    // Helper functions for dialogue management
    namespace DialogueUtils {
        // Message formatting
        String FormatNPCMessage(const String& npcName, const String& message, DialogueTone tone);
        String FormatPlayerChoice(const DialogueChoice& choice, U32 index);
        String FormatRelationshipChange(F32 change);

        // Choice analysis
        std::vector<String> ExtractTopicsFromMessage(const String& message);
        String DetermineChoiceType(const String& choiceText);
        F32 EstimateChoiceDifficulty(const String& choiceType, NPCFaction targetFaction);

        // Conversation analysis
        String SummarizeConversation(const std::vector<DialogueExchange>& exchanges);
        F32 CalculateConversationTension(const std::vector<DialogueExchange>& exchanges);
        std::vector<String> GetFrequentTopics(const std::vector<DialogueExchange>& exchanges);

        // Validation helpers
        bool IsValidChoiceText(const String& text);
        bool IsAppropriateForFaction(const String& message, NPCFaction faction);
        bool RequiresContentWarning(const String& message);

        // UI integration helpers
        String ConvertToDisplayText(const String& rawText);
        std::vector<String> WrapTextForUI(const String& text, U32 maxLineLength);
        String GetFactionColorCode(NPCFaction faction);
    }

} // namespace Angaraka::AI