// Engine/Source/Systems/Angaraka.AI/Source/AI/Public/Angaraka/AIManager.hpp
#pragma once

#include <Angaraka/Base.hpp>
#include <Angaraka/MathCore.hpp>
#include <Angaraka/AIModelResource.hpp>
#include <filesystem>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <unordered_map>

import Angaraka.Core.ResourceCache;

using namespace Angaraka::Math;

namespace Angaraka::AI {

    // Forward declarations
    struct DialogueRequest;
    struct DialogueResponse;
    struct TerrainRequest;
    struct TerrainResponse;
    struct BehaviorRequest;
    struct BehaviorResponse;

    // AI system initialization configuration
    struct AISystemConfig {
        bool enableGPUAcceleration{ true };
        size_t maxVRAMUsageMB{ 16384 };        // 16GB default for RTX 4080/4090
        F32 dialogueTimeoutMs{ 100.0f };     // Max time for dialogue inference
        F32 terrainTimeoutMs{ 5000.0f };     // Max time for terrain generation
        size_t backgroundThreadCount{ 4 };     // Threads for async operations
        String defaultFaction{ "neutral" };
        bool enablePerformanceMonitoring{ true };
    };

    // Performance monitoring data
    struct AIPerformanceMetrics {
        F32 averageDialogueTimeMs{ 0.0f };
        F32 averageTerrainTimeMs{ 0.0f };
        size_t totalVRAMUsageMB{ 0 };
        size_t totalInferences{ 0 };
        size_t failedInferences{ 0 };
        F32 gpuUtilizationPercent{ 0.0f };
    };

    // Central AI system manager
    class AIManager {
    public:
        explicit AIManager(const AISystemConfig& config = AISystemConfig{});
        ~AIManager();

        // System lifecycle
        bool Initialize(Reference<Angaraka::Core::CachedResourceManager>& cachedManager);
        void Shutdown();
        void Update(F32 deltaTime);

        // Model management
        bool LoadDialogueModel(const String& factionId, const String& modelPath);
        bool LoadTerrainModel(const String& regionType, const String& modelPath);
        bool LoadBehaviorModel(const String& behaviorType, const String& modelPath);
        void UnloadModel(const String& modelId);
        void UnloadFactionModels(const String& factionId);

        // High-level AI interfaces
        std::future<DialogueResponse> GenerateDialogue(const DialogueRequest& request);
        std::future<TerrainResponse> GenerateTerrain(const TerrainRequest& request);
        std::future<BehaviorResponse> EvaluateBehavior(const BehaviorRequest& request);

        // Synchronous versions for immediate results
        DialogueResponse GenerateDialogueSync(const DialogueRequest& request);
        BehaviorResponse EvaluateBehaviorSync(const BehaviorRequest& request);
        TerrainResponse GenerateTerrainSync(const TerrainRequest& request);

        // Faction-specific operations
        bool SwitchActiveFaction(const String& factionId);
        std::vector<String> GetLoadedFactions() const;
        bool IsFactionLoaded(const String& factionId) const;

        // Performance and monitoring
        const AIPerformanceMetrics& GetPerformanceMetrics() const { return m_performanceMetrics; }
        F32 GetVRAMUsagePercent() const;
        void OptimizeMemoryUsage();
        void EnableProfiling(bool enable) { m_profilingEnabled = enable; }

        // Configuration
        void UpdateConfig(const AISystemConfig& config) { m_config = config; }
        const AISystemConfig& GetConfig() const { return m_config; }

        // Hot-swapping for development
        bool HotSwapModel(const String& modelId, const String& newModelPath);
        void EnableHotSwapping(bool enable) { m_hotSwappingEnabled = enable; }

    private:
        AISystemConfig m_config;
        AIPerformanceMetrics m_performanceMetrics;

        // Model storage and management
        std::unordered_map<String, Reference<AIModelResource>> m_loadedModels;
        std::unordered_map<String, std::vector<String>> m_factionModels; // faction -> model IDs
        String m_activeFaction;

        // Resource management
        Reference<Angaraka::Core::CachedResourceManager> m_resourceManager;
        size_t m_currentVRAMUsage{ 0 };

        // Threading for async operations
        std::vector<std::thread> m_backgroundThreads;
        std::queue<std::function<void()>> m_taskQueue;
        std::mutex m_taskQueueMutex;
        std::condition_variable m_taskCondition;
        std::atomic<bool> m_shutdownRequested{ false };

        // Performance monitoring
        bool m_profilingEnabled{ true };
        std::chrono::steady_clock::time_point m_lastMetricsUpdate;

        // Development features
        bool m_hotSwappingEnabled{ false };
        std::unordered_map<String, std::filesystem::file_time_type> m_modelFileTimestamps;

        // Helper methods
        Reference<AIModelResource> GetModelForFaction(const String& factionId, const String& modelType);
        String GenerateModelId(const String& factionId, const String& modelType);
        bool ValidateModelCompatibility(const Reference<AIModelResource>& model);
        void UpdatePerformanceMetrics();
        void ProcessBackgroundTasks();
        void CheckForModelFileChanges();
        void CleanupUnusedModels();

        // Model loading helpers
        bool LoadModelInternal(const String& modelId, const String& modelPath, const String& factionId);
        void RegisterModelWithFaction(const String& modelId, const String& factionId);
        void UnregisterModelFromFaction(const String& modelId, const String& factionId);
    };

    // Request/Response structures for AI operations
    struct DialogueRequest {
        String factionId;
        String npcId;
        String playerMessage;
        String conversationContext;
        std::unordered_map<String, F32> emotionalState;
        std::vector<String> recentEvents;
        F32 urgency{ 0.5f }; // 0.0 = casual, 1.0 = critical
    };

    struct DialogueResponse {
        String response;
        String emotionalTone;
        std::vector<String> suggestedPlayerResponses;
        std::unordered_map<String, F32> updatedEmotionalState;
        F32 confidence{ 0.0f };
        F32 inferenceTimeMs{ 0.0f };
        bool success{ false };
    };

    struct TerrainRequest {
        String regionType;
        String factionInfluence;
        Vector3 centerPosition;
        F32 radius;
        int32_t seed;
        F32 philosophicalResonance{ 0.5f }; // How much the terrain reflects faction philosophy
        std::vector<String> requiredFeatures;
    };

    struct TerrainResponse {
        std::vector<F32> heightMap;
        std::vector<uint32_t> textureIndices;
        std::vector<Vector3> landmarkPositions;
        std::vector<String> landmarkTypes;
        F32 inferenceTimeMs{ 0.0f };
        bool success{ false };
    };

    struct BehaviorRequest {
        String factionId;
        String npcType;
        String currentSituation;
        std::vector<String> availableActions;
        std::unordered_map<String, F32> worldState;
        F32 timeConstraint{ 1.0f }; // How quickly decision is needed
    };

    struct BehaviorResponse {
        String selectedAction;
        std::vector<std::pair<String, F32>> actionConfidences;
        String reasoning;
        F32 inferenceTimeMs{ 0.0f };
        bool success{ false };
    };

} // namespace Angaraka::AI