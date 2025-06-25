#pragma once

#include <Angaraka/AIBase.hpp>
#include "Angaraka/NPCController.hpp"

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

    // Events for NPC system coordination
    struct NPCSpawnEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Spawning)
        EVENT_CLASS_TYPE(NPCSpawnEvent)

        String npcId;
        Vector3 position;
        NPCFaction faction;
        String templateId;
    };

    struct NPCDestroyEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Spawning)
        EVENT_CLASS_TYPE(NPCDestroyEvent)

        String npcId;
        String reason;
    };

    struct NPCSystemUpdateEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Spawning)
        EVENT_CLASS_TYPE(NPCSystemUpdateEvent)

        U32 activeNPCCount;
        U32 visibleNPCCount;
        F32 totalUpdateTime;
        F32 averageUpdateTime;
    };

    struct PlayerLocationUpdateEvent : public Angaraka::Events::Event {
    public:
        EVENT_CLASS_CATEGORY(Angaraka::Events::EventCategory::Spawning)
        EVENT_CLASS_TYPE(PlayerLocationUpdateEvent)

        Vector3 position;
        Vector3 direction;
        F32 viewDistance;
    };

    // NPC template for easy spawning
    struct NPCTemplate {
        String templateId;
        String displayName;
        NPCFaction faction{ NPCFaction::Neutral };
        NPCType npcType{ NPCType::Civilian };
        NPCPersonality personality;
        String meshResourceId;
        String textureResourceId;
        String dialogueModelId;
        String behaviorModelId;
        F32 interactionRange{ 3.0f };
        InteractionType defaultInteraction{ InteractionType::Dialogue };
        std::vector<String> knownTopics;
        std::unordered_map<String, F32> topicExpertise;
        String description;

        NPCTemplate() = default;

        // Constructor with defaults
        explicit NPCTemplate(const String& id, const String& name, NPCFaction fac = NPCFaction::Neutral)
            : templateId(id), displayName(name), faction(fac) {
        }
    };

    // NPC spawning parameters
    struct NPCSpawnParams {
        String npcId;
        String templateId;
        Vector3 position{ Vector3::Zero };
        Vector3 rotation{ Vector3::Zero };
        Vector3 scale{ Vector3::One };
        bool isActive{ true };
        bool isVisible{ true };

        // Override template values if needed
        String customDisplayName;
        NPCFaction customFaction{ NPCFaction::Count }; // Count means use template
        std::unordered_map<String, F32> personalityOverrides;
        std::unordered_map<String, F32> relationshipOverrides;
    };

    // Performance and culling settings
    struct NPCManagerSettings {
        F32 maxUpdateDistance{ 200.0f };           // NPCs beyond this distance skip updates
        F32 maxRenderDistance{ 150.0f };           // NPCs beyond this distance are not rendered
        F32 maxInteractionDistance{ 50.0f };       // NPCs beyond this distance cannot be interacted with
        U32 maxActiveNPCs{ 100 };             // Maximum NPCs that can be active simultaneously
        U32 maxUpdatesPerFrame{ 20 };         // Maximum NPC updates per frame for performance
        F32 updateIntervalMultiplier{ 1.0f };      // Global multiplier for all AI update intervals
        bool enableDistanceCulling{ true };        // Enable distance-based performance optimization
        bool enableFrustumCulling{ false };        // Enable view frustum culling (requires camera integration)
        bool enableBatchUpdates{ true };           // Process NPCs in batches for better performance

        // Debug settings
        bool enableDebugLogging{ false };
        bool enablePerformanceMetrics{ true };
        bool enableStateLogging{ false };
    };

    // Multi-NPC coordination and system integration
    class NPCManager {
    public:
        explicit NPCManager(
            Reference<Angaraka::Core::CachedResourceManager> resourceManager,
            Reference<Angaraka::AI::AIManager> aiManager,
            Reference<Angaraka::DirectX12GraphicsSystem> graphicsSystem
        );

        ~NPCManager();

        // Core lifecycle
        bool Initialize(const NPCManagerSettings& settings = NPCManagerSettings{});
        void Update(F32 deltaTime);
        void Render(F32 deltaTime); // Coordinate NPC rendering with graphics system
        void Shutdown();

        // NPC template management
        bool RegisterTemplate(const NPCTemplate& npcTemplate);
        bool UnregisterTemplate(const String& templateId);
        const NPCTemplate* GetTemplate(const String& templateId) const;
        std::vector<String> GetRegisteredTemplates() const;

        // NPC lifecycle management
        bool SpawnNPC(const NPCSpawnParams& spawnParams);
        bool SpawnNPC(const String& npcId, const String& templateId, const Vector3& position);
        bool DestroyNPC(const String& npcId, const String& reason = "Manual");
        void DestroyAllNPCs();

        // NPC access and queries
        NPCController* GetNPC(const String& npcId);
        const NPCController* GetNPC(const String& npcId) const;
        std::vector<NPCController*> GetNPCsInRange(const Vector3& position, F32 range);
        std::vector<NPCController*> GetNPCsByFaction(NPCFaction faction);
        std::vector<NPCController*> GetNPCsByState(NPCState state);
        std::vector<NPCController*> GetInteractableNPCs(const Vector3& playerPosition);

        // Bulk operations
        void SetAllNPCsActive(bool active);
        void SetNPCsActiveInRange(const Vector3& center, F32 range, bool active);
        void UpdateAllNPCDistances(const Vector3& playerPosition);
        void ChangeStateForFaction(NPCFaction faction, NPCState newState, const String& reason);

        // Player integration
        void UpdatePlayerLocation(const Vector3& position, const Vector3& direction = Vector3::Zero);
        void TriggerPlayerInteraction(const String& npcId, const String& action, InteractionType type = InteractionType::Dialogue);
        std::vector<String> GetNearbyInteractableNPCs(F32 range = 5.0f) const;

        // Bundle and configuration loading
        bool LoadNPCsFromBundle(const String& bundleId);
        bool SaveNPCsToBundle(const String& bundleId) const;
        bool LoadTemplatesFromConfig(const String& configPath);

        // Performance and optimization
        void OptimizeNPCUpdates();
        void CullDistantNPCs();
        void EnableNPCsInFrustum(const Vector3& cameraPos, const Vector3& cameraDir, F32 fov);

        // Settings management
        void UpdateSettings(const NPCManagerSettings& newSettings);
        const NPCManagerSettings& GetSettings() const { return m_settings; }

        // Statistics and debugging
        U32 GetActiveNPCCount() const;
        U32 GetVisibleNPCCount() const;
        U32 GetTotalNPCCount() const;
        F32 GetAverageUpdateTime() const;
        String GetPerformanceReport() const;
        void LogSystemStatus() const;

        // Event callbacks for extensibility
        using NPCSpawnCallback = std::function<void(NPCController*)>;
        using NPCDestroyCallback = std::function<void(const String&)>;
        using NPCUpdateCallback = std::function<void(const std::vector<NPCController*>&, F32)>;

        void SetSpawnCallback(NPCSpawnCallback callback) { m_spawnCallback = callback; }
        void SetDestroyCallback(NPCDestroyCallback callback) { m_destroyCallback = callback; }
        void SetUpdateCallback(NPCUpdateCallback callback) { m_updateCallback = callback; }

    private:
        // System integration
        Reference<Angaraka::Core::CachedResourceManager> m_resourceManager;
        Reference<Angaraka::AI::AIManager> m_aiManager;
        Reference<Angaraka::DirectX12GraphicsSystem> m_graphicsSystem;

        // NPC storage and management
        std::unordered_map<String, std::unique_ptr<NPCController>> m_npcs;
        std::unordered_map<String, NPCTemplate> m_templates;
        mutable std::mutex m_npcMutex; // Thread safety for NPC access

        // Settings and configuration
        NPCManagerSettings m_settings;
        bool m_isInitialized{ false };

        // Player state tracking
        Vector3 m_playerPosition{ Vector3::Zero };
        Vector3 m_playerDirection{ Vector3::Zero };
        F32 m_playerViewDistance{ 100.0f };

        // Performance tracking
        F32 m_totalUpdateTime{ 0.0f };
        U32 m_updateCount{ 0 };
        U32 m_frameUpdateCount{ 0 };
        std::chrono::steady_clock::time_point m_lastPerformanceReport;

        // Update optimization
        std::vector<NPCController*> m_activeNPCs;      // NPCs that need regular updates
        std::vector<NPCController*> m_visibleNPCs;     // NPCs visible to player
        std::vector<NPCController*> m_interactableNPCs; // NPCs in interaction range
        U32 m_currentUpdateIndex{ 0 };            // For batch processing

        // Event callbacks
        NPCSpawnCallback m_spawnCallback;
        NPCDestroyCallback m_destroyCallback;
        NPCUpdateCallback m_updateCallback;

        // Event handling
        void SubscribeToEvents();
        void UnsubscribeFromEvents();
        void OnPlayerMovement(const Vector3& position);
        void OnNPCInteraction(const NPCInteractionEvent& interaction);
        void OnNPCStateChange(const NPCStateChangeEvent& stateChange);

        // Internal NPC management
        bool CreateNPCFromTemplate(const NPCSpawnParams& spawnParams);
        void UpdateNPCLists();
        void UpdateNPCBatch(const std::vector<NPCController*>& npcs, F32 deltaTime);
        void ProcessNPCEvents();

        // Performance optimization helpers
        void CategorizeNPCs();
        bool ShouldUpdateNPC(const NPCController* npc) const;
        bool ShouldRenderNPC(const NPCController* npc) const;
        bool IsNPCInView(const NPCController* npc) const;

        // Resource management helpers
        void PreloadNPCResources(const NPCTemplate& npcTemplate);
        void CleanupUnusedResources();

        // Validation and error handling
        bool ValidateSpawnParams(const NPCSpawnParams& params) const;
        bool ValidateTemplate(const NPCTemplate& npcTemplate) const;
        void LogNPCError(const String& operation, const String& npcId, const String& error) const;

        // Debug and diagnostics
        void UpdatePerformanceMetrics(F32 deltaTime);
        void LogPerformanceReport() const;
        String FormatNPCStatus(const NPCController* npc) const;
    };

    // Helper functions for NPC management
    namespace NPCManagerUtils {
        // Template creation helpers
        NPCTemplate CreateCivilianTemplate(const String& templateId, NPCFaction faction);
        NPCTemplate CreateGuardTemplate(const String& templateId, NPCFaction faction);
        NPCTemplate CreateMerchantTemplate(const String& templateId, NPCFaction faction);
        NPCTemplate CreateScholarTemplate(const String& templateId, NPCFaction faction);

        // Spawning helpers
        std::vector<NPCSpawnParams> GenerateRandomNPCs(U32 count, const Vector3& center, F32 radius);
        NPCSpawnParams CreateSpawnParams(const String& npcId, const String& templateId, const Vector3& position);

        // Performance helpers
        std::vector<NPCController*> SortNPCsByDistance(const std::vector<NPCController*>& npcs, const Vector3& referencePoint);
        std::vector<NPCController*> FilterNPCsByRange(const std::vector<NPCController*>& npcs, const Vector3& center, F32 range);

        // Configuration helpers
        bool SaveTemplatesToFile(const std::unordered_map<String, NPCTemplate>& templates, const String& filePath);
        bool LoadTemplatesFromFile(std::unordered_map<String, NPCTemplate>& templates, const String& filePath);

        // Debug helpers
        String GetNPCStatusSummary(const std::vector<NPCController*>& npcs);
        void LogNPCDistribution(const std::vector<NPCController*>& npcs, const Vector3& playerPos);
    }

} // namespace Angaraka::AI