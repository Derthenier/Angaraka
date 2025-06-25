#pragma once

#include <Angaraka/AIBase.hpp>
#include "Angaraka/NPCComponent.hpp"

using namespace Angaraka::Math;

// Forward declarations for integration
namespace Angaraka::Core {
    class CachedResourceManager;
}

namespace Angaraka::Events {
    class EventManager;
}

namespace Angaraka::AI {
    class AIManager;
}

namespace Angaraka {
    class DirectX12GraphicsSystem;
}

import Angaraka.Core.Events;

namespace Angaraka::AI {

    // Events for NPC interactions
    struct NPCInteractionEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::NPCInteraction)
        EVENT_CLASS_TYPE(NPCInteractionEvent)

        String npcId;
        String playerAction;
        Vector3 playerPosition;
        F32 interactionDistance;
        InteractionType interactionType;
    };

    struct NPCStateChangeEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::NPCInteraction)
        EVENT_CLASS_TYPE(NPCStateChangeEvent)

        String npcId;
        NPCState oldState;
        NPCState newState;
        String reason;
    };

    struct NPCDialogueRequestEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::NPCInteraction)
        EVENT_CLASS_TYPE(NPCDialogueRequestEvent)

        String npcId;
        String playerMessage;
        String conversationContext;
    };

    struct NPCDialogueResponseEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::NPCInteraction)
        EVENT_CLASS_TYPE(NPCDialogueResponseEvent)

        String npcId;
        String npcResponse;
        String emotionalTone;
        std::vector<String> playerOptions;
        bool endConversation;
    };

    // Callback types for NPC behavior
    using NPCUpdateCallback = std::function<void(NPCComponent&, F32)>;
    using NPCInteractionCallback = std::function<void(NPCComponent&, const NPCInteractionEvent&)>;
    using NPCStateChangeCallback = std::function<void(NPCComponent&, NPCState, NPCState)>;

    // Individual NPC behavior controller
    class NPCController {
    public:
        explicit NPCController(
            const String& npcId,
            Reference<Angaraka::Core::CachedResourceManager> resourceManager,
            Reference<Angaraka::AI::AIManager> aiManager,
            Reference<Angaraka::DirectX12GraphicsSystem> graphicsSystem
        );

        ~NPCController();

        // Core lifecycle
        bool Initialize(const NPCComponent& initialData);
        void Update(F32 deltaTime);
        void Shutdown();

        // NPC data access
        NPCComponent& GetNPCData() { return m_npcData; }
        const NPCComponent& GetNPCData() const { return m_npcData; }
        const String& GetNPCId() const { return m_npcData.npcId; }

        // State management
        void ChangeState(NPCState newState, const String& reason = "");
        bool CanChangeState(NPCState newState) const;
        NPCState GetCurrentState() const { return m_npcData.currentState; }

        // Position and world interaction
        void SetPosition(const Vector3& position);
        void SetRotation(const Vector3& rotation);
        Vector3 GetPosition() const { return m_npcData.position; }
        Matrix4x4 GetWorldMatrix() const { return m_npcData.GetWorldMatrix(); }

        // Player interaction
        bool IsPlayerInRange(const Vector3& playerPosition) const;
        bool CanInteractWithPlayer() const;
        void TriggerInteraction(const NPCInteractionEvent& interaction);

        // AI behavior integration
        void RequestAIDecision(const String& situation, const std::vector<String>& availableActions);
        void ProcessDialogueRequest(const String& playerMessage, const String& context);

        // Rendering integration
        void SetMeshResource(const String& meshResourceId);
        void SetTextureResource(const String& textureResourceId);
        bool IsVisibleToPlayer() const { return m_npcData.isVisible && m_npcData.isActive; }

        // Relationship management
        void UpdateRelationship(F32 trustDelta, F32 respectDelta, F32 fearDelta = 0.0f);
        const NPCRelationship& GetRelationship() const { return m_npcData.relationship; }

        // Knowledge and memory
        void AddKnownTopic(const String& topic, F32 expertise = 0.5f);
        void RecordEvent(const String& event);
        bool HasKnowledge(const String& topic) const;

        // Callback registration for extensibility
        void SetUpdateCallback(NPCUpdateCallback callback) { m_updateCallback = callback; }
        void SetInteractionCallback(NPCInteractionCallback callback) { m_interactionCallback = callback; }
        void SetStateChangeCallback(NPCStateChangeCallback callback) { m_stateChangeCallback = callback; }

        // Performance optimization
        void SetActive(bool active);
        bool IsActive() const { return m_npcData.isActive; }
        void UpdateDistanceToPlayer(const Vector3& playerPosition);

        // Debug and diagnostics
        String GetDebugInfo() const;
        void LogCurrentState() const;

    private:
        // Core data
        NPCComponent m_npcData;

        // System integration
        Reference<Angaraka::Core::CachedResourceManager> m_resourceManager;
        Reference<Angaraka::AI::AIManager> m_aiManager;
        Reference<Angaraka::DirectX12GraphicsSystem> m_graphicsSystem;

        // Behavior callbacks
        NPCUpdateCallback m_updateCallback;
        NPCInteractionCallback m_interactionCallback;
        NPCStateChangeCallback m_stateChangeCallback;

        // Internal state
        bool m_isInitialized{ false };
        bool m_pendingAIDecision{ false };
        String m_currentAISituation;
        std::vector<String> m_currentAIActions;

        // Performance tracking
        std::chrono::steady_clock::time_point m_lastUpdate;
        F32 m_totalUpdateTime{ 0.0f };
        U32 m_updateCount{ 0 };

        // Event handling
        void SubscribeToEvents();
        void UnsubscribeFromEvents();
        void OnPlayerMovement(const Vector3& playerPosition);
        void OnPlayerInteraction(const NPCInteractionEvent& interaction);
        void OnAIDecisionResponse(const String& npcId, const String& selectedAction, const String& reasoning);
        void OnDialogueResponse(const String& npcId, const String& response, const String& emotionalTone);

        // Internal behavior methods
        void UpdateAIBehavior(F32 deltaTime);
        void UpdateIdleBehavior(F32 deltaTime);
        void UpdateConversationBehavior(F32 deltaTime);
        void UpdatePatrolBehavior(F32 deltaTime);
        void UpdateAlertBehavior(F32 deltaTime);

        // State transition validation
        bool ValidateStateTransition(NPCState fromState, NPCState toState) const;
        void OnStateChanged(NPCState oldState, NPCState newState);

        // AI integration helpers
        void PrepareAIRequest();
        String BuildConversationContext() const;
        std::unordered_map<String, F32> GetCurrentWorldState() const;

        // Relationship helpers
        void ProcessPlayerAction(const String& action);
        F32 CalculateRelationshipModifier(const String& action) const;

        // Performance helpers
        bool ShouldSkipUpdate() const;
        void UpdatePerformanceMetrics(F32 deltaTime);

        // Debug helpers
        void ValidateNPCData() const;
        void LogStateTransition(NPCState oldState, NPCState newState, const String& reason) const;
    };

    // Helper functions for NPC management
    namespace NPCUtils {
        // Distance calculations
        F32 CalculateDistance(const Vector3& pos1, const Vector3& pos2);
        bool IsPositionInRange(const Vector3& pos1, const Vector3& pos2, F32 range);

        // Faction utilities
        bool AreFactionsHostile(NPCFaction faction1, NPCFaction faction2);
        F32 GetFactionRelationshipModifier(NPCFaction npcFaction, NPCFaction playerFaction = NPCFaction::Neutral);

        // State utilities
        bool IsInteractiveState(NPCState state);
        bool IsMovementState(NPCState state);
        std::vector<NPCState> GetValidTransitions(NPCState currentState);

        // AI utilities
        String FormatPersonalityForAI(const NPCPersonality& personality);
        String FormatRelationshipForAI(const NPCRelationship& relationship);
        String FormatKnowledgeForAI(const NPCKnowledge& knowledge);
    }

} // namespace Angaraka::AI