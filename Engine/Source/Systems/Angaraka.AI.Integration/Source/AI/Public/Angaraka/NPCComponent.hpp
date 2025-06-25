#pragma once

#include <Angaraka/AIBase.hpp>

namespace Angaraka::AI {

    // Forward declarations
    class AIManager;
    class NPCController;

    // NPC States for behavior management
    enum class NPCState : U32 {
        Idle = 0,
        Patrol,
        Conversation,
        Alert,
        Working,
        Sleeping,
        Following,
        Hostile,
        Count
    };

    // NPC Types for different AI behaviors
    enum class NPCType : U32 {
        Civilian = 0,
        Guard,
        Merchant,
        Scholar,
        Leader,
        Worker,
        Count
    };

    // NPC interaction types
    enum class InteractionType : U32 {
        None = 0,
        Dialogue,
        Trade,
        Quest,
        Information,
        Hostile,
        Count
    };

    // NPC faction affiliations (matches your AI system)
    enum class NPCFaction : U32 {
        Neutral = 0,
        Ashvattha,      // Traditional/spiritual faction
        Vaikuntha,      // Tech/corporate faction  
        YugaStriders,   // Rebel/freedom faction
        Count
    };

    // NPC personality traits for AI behavior
    struct NPCPersonality {
        F32 aggressiveness{ 0.5f };     // 0.0 = peaceful, 1.0 = hostile
        F32 curiosity{ 0.5f };          // 0.0 = aloof, 1.0 = inquisitive
        F32 trustfulness{ 0.5f };       // 0.0 = suspicious, 1.0 = trusting
        F32 helpfulness{ 0.5f };        // 0.0 = selfish, 1.0 = altruistic
        F32 intelligence{ 0.5f };       // 0.0 = simple, 1.0 = complex reasoning
        F32 loyalty{ 0.5f };            // 0.0 = independent, 1.0 = faction loyal
    };

    // NPC relationship state with player
    struct NPCRelationship {
        F32 trust{ 0.5f };              // Player trust level
        F32 respect{ 0.5f };            // Player respect level
        F32 fear{ 0.0f };               // Fear of player
        F32 affection{ 0.5f };          // Personal liking
        U32 conversationCount{ 0 }; // Number of interactions
        std::chrono::steady_clock::time_point lastInteraction;
    };

    // NPC knowledge and memories
    struct NPCKnowledge {
        std::vector<String> knownTopics;        // Topics NPC can discuss
        std::vector<String> recentEvents;       // Recent world events NPC knows
        std::unordered_map<String, F32> topicExpertise; // Topic -> expertise level
        std::vector<String> secrets;            // Information NPC won't share easily
        String lastPlayerAction;                // What player did recently
    };

    // Core NPC data component
    struct NPCComponent {
        // Identity
        String npcId;                           // Unique identifier
        String displayName;                     // Name shown to player
        String description;                     // Brief description

        // Faction and Type
        NPCFaction faction{ NPCFaction::Neutral };
        NPCType npcType{ NPCType::Civilian };

        // AI Behavior
        NPCState currentState{ NPCState::Idle };
        NPCState previousState{ NPCState::Idle };
        NPCPersonality personality;
        NPCRelationship relationship;
        NPCKnowledge knowledge;

        // World Position and Rendering
        Math::Vector3 position{ Math::Vector3::Zero };
        Math::Vector3 rotation{ Math::Vector3::Zero };  // Euler angles in radians
        Math::Vector3 scale{ Math::Vector3::One };
        String meshResourceId;                  // Mesh for rendering
        String textureResourceId;               // Texture for rendering

        // Interaction
        InteractionType availableInteraction{ InteractionType::Dialogue };
        F32 interactionRange{ 3.0f };           // Distance for player interaction
        bool isInteractable{ true };
        bool isVisible{ true };

        // AI Model Configuration
        String dialogueModelId;                 // AI model for conversations
        String behaviorModelId;                 // AI model for behavior decisions
        F32 aiUpdateInterval{ 1.0f };           // Seconds between AI updates
        std::chrono::steady_clock::time_point lastInteraction;
        std::chrono::steady_clock::time_point lastAIUpdate;

        // Performance and State
        bool isActive{ true };                  // Whether NPC is actively updated
        F32 distanceToPlayer{ 0.0f };           // Cached distance for optimization
        bool isInPlayerView{ false };           // Cached visibility for optimization

        // Constructor with sensible defaults
        NPCComponent(const String& id, const String& name, NPCFaction fac = NPCFaction::Neutral)
            : npcId(id), displayName(name), faction(fac) {
            lastInteraction = std::chrono::steady_clock::now();
            lastAIUpdate = std::chrono::steady_clock::now();

            // Set faction-specific defaults
            SetFactionDefaults();
        }

        // Helper method to set faction-specific personality defaults
        void SetFactionDefaults() {
            switch (faction) {
            case NPCFaction::Ashvattha:
                personality.intelligence = 0.8f;
                personality.trustfulness = 0.7f;
                personality.helpfulness = 0.8f;
                personality.loyalty = 0.9f;
                personality.aggressiveness = 0.2f;
                dialogueModelId = "ashvattha_dialogue";
                behaviorModelId = "ashvattha_behavior";
                break;

            case NPCFaction::Vaikuntha:
                personality.intelligence = 0.9f;
                personality.trustfulness = 0.4f;
                personality.helpfulness = 0.5f;
                personality.loyalty = 0.8f;
                personality.aggressiveness = 0.3f;
                dialogueModelId = "vaikuntha_dialogue";
                behaviorModelId = "vaikuntha_behavior";
                break;

            case NPCFaction::YugaStriders:
                personality.intelligence = 0.7f;
                personality.trustfulness = 0.6f;
                personality.helpfulness = 0.7f;
                personality.loyalty = 0.6f;
                personality.aggressiveness = 0.6f;
                dialogueModelId = "yuga_striders_dialogue";
                behaviorModelId = "yuga_striders_behavior";
                break;

            default: // Neutral
                personality.intelligence = 0.6f;
                personality.trustfulness = 0.5f;
                personality.helpfulness = 0.5f;
                personality.loyalty = 0.5f;
                personality.aggressiveness = 0.3f;
                dialogueModelId = "neutral_dialogue";
                behaviorModelId = "neutral_behavior";
                break;
            }
        }

        // Helper methods for state management
        bool ShouldUpdateAI() const {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAIUpdate);
            return elapsed.count() >= (aiUpdateInterval * 1000.0f);
        }

        void MarkAIUpdated() {
            lastAIUpdate = std::chrono::steady_clock::now();
        }

        Math::Matrix4x4 GetWorldMatrix() const {
            return Math::Transform(position, Math::Quaternion::FromEuler(rotation), scale).ToMatrix();
        }

        // Get faction as string for AI system
        const char* GetFactionString() const {
            switch (faction) {
            case NPCFaction::Ashvattha: return "ashvattha";
            case NPCFaction::Vaikuntha: return "vaikuntha";
            case NPCFaction::YugaStriders: return "yuga_striders";
            default: return "neutral";
            }
        }

        // Get state as string for debugging
        const char* GetStateString() const {
            switch (currentState) {
            case NPCState::Idle: return "Idle";
            case NPCState::Patrol: return "Patrol";
            case NPCState::Conversation: return "Conversation";
            case NPCState::Alert: return "Alert";
            case NPCState::Working: return "Working";
            case NPCState::Sleeping: return "Sleeping";
            case NPCState::Following: return "Following";
            case NPCState::Hostile: return "Hostile";
            default: return "Unknown";
            }
        }
    };

} // namespace Angaraka::AI