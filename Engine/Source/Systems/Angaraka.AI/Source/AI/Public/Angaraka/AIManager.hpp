// Engine/Source/Systems/Angaraka.AI/Source/AI/Public/Angaraka/AIManager.hpp
#pragma once

#include <Angaraka/Base.hpp>
#include <Angaraka/AIModelResource.hpp>
#include <Angaraka/Tokenizer.hpp>
#include <unordered_map>
#include <memory>
#include <future>
#include <queue>
#include <thread>

import Angaraka.Core.Resources;
import Angaraka.Core.Config;
import Angaraka.Core.ResourceCache;
import Angaraka.Graphics.DirectX12;
import Angaraka.Math.Vector3;

namespace Angaraka::AI {

    // Forward declarations
    struct DialogueRequest;
    struct DialogueResponse;
    struct TerrainRequest;
    struct TerrainResponse;
    struct BehaviorRequest;
    struct BehaviorResponse;

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
    private:
        // Simple faction configs matching actual JSON structure
        struct FactionConfig {
            String factionId;
            String displayName;                     // "name" in JSON
            String description;                     // "ideology" in JSON  
            String promptTemplate;                  // "prompt_template" in JSON
            std::vector<String> personalityTraits;  // "personality_traits" in JSON
            std::vector<String> keyVocabulary;      // "key_vocabulary" in JSON
        };

    public:
        explicit AIManager(const Config::AISystemConfig& config = Config::AISystemConfig{});
        ~AIManager();

        // System lifecycle
        bool Initialize(Reference<Angaraka::DirectX12GraphicsSystem> graphicsSystem, Reference<Angaraka::Core::CachedResourceManager> resourceManager);
        void Shutdown();
        void Update(F32 deltaTime);

        // Model management - NEW STRUCTURE
        Scope<Ort::Session> CreateSession(const WString& modelPath, bool sharedModel = false);
        bool LoadSharedDialogueModel(const String& modelPath);
        bool LoadFactionConfigs(const String& modelsDirectory);
        bool LoadSharedTokenizer(const String& tokenizerPath);
        void UnloadSharedModel();

        // Legacy methods (deprecated)
        [[deprecated("Use LoadSharedDialogueModel instead")]]
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
        TerrainResponse GenerateTerrainSync(const TerrainRequest& request);
        BehaviorResponse EvaluateBehaviorSync(const BehaviorRequest& request);

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
        void UpdateConfig(const Config::AISystemConfig& config) { m_config = config; }
        const Config::AISystemConfig& GetConfig() const { return m_config; }

        // Shared AI system access
        Reference<Tokenizer> GetSharedTokenizer() const { return m_sharedTokenizer; }
        Reference<FactionConfig> GetFactionConfig(const String& factionId);
        std::vector<String> GetAvailableFactions() const;

        // Legacy tokenizer management (deprecated)
        [[deprecated("Use LoadSharedTokenizer instead")]]
        bool LoadTokenizerForFaction(const String& factionId, const String& tokenizerPath);
        [[deprecated("Use GetSharedTokenizer instead")]]
        Reference<Tokenizer> GetTokenizerForFaction(const String& factionId);

        // Hot-swapping for development
        bool HotSwapModel(const String& modelId, const String& newModelPath);
        void EnableHotSwapping(bool enable) { m_hotSwappingEnabled = enable; }

    private:
        Config::AISystemConfig m_config;
        AIPerformanceMetrics m_performanceMetrics;
        Reference<Angaraka::DirectX12GraphicsSystem> m_graphicsSystem;

        // Model storage and management
        Reference<AIModelResource> m_sharedDialogueModel;   // Single 600MB model for all factions
        Reference<Tokenizer> m_sharedTokenizer;             // Single tokenizer for all factions
        std::vector<I64> m_lastTokenSequence;

        std::unordered_map<String, Reference<FactionConfig>> m_factionConfigs;

        // Legacy model storage (for terrain/behavior models)
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

        // Output processing helpers
        DialogueResponse ProcessDialogueOutputs(const std::vector<Ort::Value>& outputs, Reference<AIModelResource> model, const DialogueRequest& request);
        TerrainResponse ProcessTerrainOutputs(const std::vector<Ort::Value>& outputs, Reference<AIModelResource> model, const TerrainRequest& request);
        BehaviorResponse ProcessBehaviorOutputs(const std::vector<Ort::Value>& outputs, Reference<AIModelResource> model, const BehaviorRequest& request);

        // Tensor extraction helpers
        std::vector<I64> ExtractInt64Tensor(const Ort::Value& tensor);
        std::vector<F32> ExtractFloatTensor(const Ort::Value& tensor);
        String ExtractStringTensor(const Ort::Value& tensor);
        std::vector<String> ExtractStringTensorArray(const Ort::Value& tensor);

        // Dialogue processing helpers
        String DecodeTokensToText(const std::vector<I64>& tokens, const String& factionId);
        String DecodeTokensBasic(const std::vector<I64>& tokens, const String& factionId);
        String DecodeTokensEnhanced(const std::vector<I64>& tokens, const String& factionId);
        String DecodeGPTToken(I64 tokenId);
        String DecodeGPTTokenEnhanced(I64 tokenId);
        String DecodeSpecificToken(I64 tokenId);
        String EnhanceResponse(const String& baseText, const String& factionId);
        String CleanGeneratedText(const String& rawText, const String& factionId);
        bool IsGarbageText(const String& text);
        String ClassifyEmotion(const std::vector<F32>& emotionScores);
        void ApplyFactionDialogueStyle(DialogueResponse& response, const String& factionId);
        String GenerateFallbackResponse(const DialogueRequest& request);
        String EnhanceDialogueResponse(const String& baseResponse, const String& factionId);
        String RemoveRepetitiveWords(const String& text);

        // Logits processing helpers
        std::vector<I64> ConvertLogitsToTokens(const std::vector<F32>& logits);
        F32 CalculateConfidenceFromLogits(const std::vector<F32>& logits);
        std::vector<F32> ApplySoftmax(const std::vector<F32>& logits);

        // Shared model helpers
        std::vector<I64> CreateDialogueTokens(const String& fullPrompt, const String& factionId);
        std::vector<F32> CreateFactionContextVector(const String& factionId, const DialogueRequest& request);

        // New implementation helpers  
        bool LoadFactionConfigFile(const String& factionId, const String& configPath);
        String BuildFactionPrompt(Reference<FactionConfig> config, const DialogueRequest& request);
        std::vector<Ort::Value> CreateDialogueInputs(const String& prompt, const DialogueRequest& request, const std::vector<String>& inputNames, const std::vector<std::vector<I64>>& inputShapes);
        std::vector<I64> TokenizePrompt(const String& prompt);
        Ort::Value CreateValidatedInt64Tensor(const std::vector<I64>& data, const std::vector<I64>& shape, const String& tensorName);
        void DiagnoseInferenceInputs(const std::vector<Ort::Value>& inputs, const std::vector<String>& inputNames);
        bool ValidateTokenSequence(const std::vector<I64>& tokens);
        void ApplyFactionPostProcessing(DialogueResponse& response, Reference<FactionConfig> config);
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
        Math::Vector3 centerPosition;
        F32 radius;
        I32 seed;
        F32 philosophicalResonance{ 0.5f }; // How much the terrain reflects faction philosophy
        std::vector<String> requiredFeatures;
    };

    struct TerrainResponse {
        std::vector<F32> heightMap;
        std::vector<U32> textureIndices;
        std::vector<Math::Vector3> landmarkPositions;
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