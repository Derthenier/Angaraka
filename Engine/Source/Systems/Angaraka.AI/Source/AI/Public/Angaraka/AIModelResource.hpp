// Engine/Source/Systems/Angaraka.AI/Source/AI/Public/Angaraka/AIModelResource.hpp
// UPDATED VERSION for Shared Model Architecture

#pragma once

#include <Angaraka/Base.hpp>
#include <onnxruntime_cxx_api.h>
#include <unordered_map>
#include <atomic>

import Angaraka.Core.Resources;

namespace Angaraka::AI {

    // UPDATED: Enhanced metadata for shared models
    struct AIModelMetadata {
        String modelType;          // "dialogue", "terrain", "behavior"
        String architecture;       // "shared_multi_faction", "single_faction", etc.
        std::vector<String> supportedFactions; // Which factions this model supports
        std::vector<String> inputNames;
        std::vector<String> outputNames;
        std::vector<std::vector<I64>> inputShapes;
        std::vector<std::vector<I64>> outputShapes;
        F32 maxInferenceTimeMs;       // Performance target
        size_t maxMemoryMB;             // Memory usage limit
        size_t optimalBatchSize;        // NEW: Optimal batch size for shared model
        bool supportsAsyncInference;    // NEW: Whether model supports async operations
        String description;        // Human-readable description
        String version;            // NEW: Model version for compatibility checking
    };

    // UPDATED: Enhanced model resource for shared architecture
    class AIModelResource : public Angaraka::Core::Resource {
    public:
        explicit AIModelResource(const String& id);
        ~AIModelResource();

        AGK_RESOURCE_TYPE_ID(AIModelResource);

        // Implement Resource interface
        bool Load(const String& filePath, void* context = nullptr) override;
        void Unload() override;

        // UPDATED: Enhanced inference methods for shared models
        std::vector<Ort::Value> RunInference(const std::vector<Ort::Value>& inputs);
        std::vector<Ort::Value> RunInferenceAsync(const std::vector<Ort::Value>& inputs);

        // NEW: Batch inference for multiple requests (efficient for shared model)
        std::vector<std::vector<Ort::Value>> RunBatchInference(const std::vector<std::vector<Ort::Value>>& batchInputs);

        // NEW: Faction-aware inference (adds logging/metrics per faction)
        std::vector<Ort::Value> RunFactionInference(const std::vector<Ort::Value>& inputs, const String& factionId);

        // Getters
        const AIModelMetadata& GetMetadata() const { return m_metadata; }
        bool IsLoaded() const { return m_session != nullptr; }
        bool IsSharedModel() const { return m_metadata.architecture == "shared_multi_faction"; }
        bool SupportsFaction(const String& factionId) const;

        // NEW: Performance monitoring per faction
        F32 GetFactionInferenceTimeMs(const String& factionId) const;
        size_t GetFactionInferenceCount(const String& factionId) const;
        void ResetFactionMetrics(const String& factionId);

        // Performance monitoring
        F32 GetLastInferenceTimeMs() const { return m_lastInferenceTimeMs; }
        size_t GetMemoryUsageMB() const { return m_memoryUsageMB; }

        // Resource interface compliance
        size_t GetSizeInBytes() const override { return m_memoryUsageMB * 1024 * 1024; }

        // Tensor creation helpers (moved from static to instance methods for better error handling)
        static Ort::Value CreateFloatTensor(const std::vector<F32>& data, const std::vector<I64>& shape);
        static Ort::Value CreateInt64Tensor(const std::vector<I64>& data, const std::vector<I64>& shape);
        static Ort::Value CreateStringTensor(const std::vector<String>& data, const std::vector<I64>& shape);

        // Model introspection
        std::vector<String> GetInputNames() const;
        std::vector<String> GetOutputNames() const;
        std::vector<std::vector<I64>> GetInputShapes() const;
        std::vector<std::vector<I64>> GetOutputShapes() const;

        // NEW: Model warming/optimization for shared models
        bool WarmupModel();
        void OptimizeForSharedUsage();

    private:
        Scope<Ort::Session> m_session;
        Scope<Ort::Env> m_environment;
        Scope<Ort::SessionOptions> m_sessionOptions;
        AIModelMetadata m_metadata;

        // Performance tracking
        mutable F32 m_lastInferenceTimeMs{ 0.0f };
        mutable size_t m_memoryUsageMB{ 0 };

        // NEW: Per-faction performance tracking for shared models
        mutable std::unordered_map<String, F32> m_factionInferenceTimes;
        mutable std::unordered_map<String, size_t> m_factionInferenceCounts;
        mutable std::unordered_map<String, std::chrono::steady_clock::time_point> m_factionLastUsed;

        // Thread safety for async inference
        mutable std::mutex m_inferenceMutex;
        mutable std::mutex m_metricsMutex;

        // NEW: Batch processing support
        std::atomic<bool> m_batchProcessingEnabled{ false };
        size_t m_optimalBatchSize{ 1 };

        // Helper methods
        bool LoadMetadata(const String& metadataPath);
        bool ValidateModelInputsOutputs();
        void UpdatePerformanceMetrics(F32 inferenceTimeMs, size_t memoryUsage) const;

        // NEW: Faction-specific metrics
        void UpdateFactionMetrics(const String& factionId, F32 inferenceTimeMs) const;
        void CleanupOldFactionMetrics() const;
    };

    // UPDATED: Factory with shared model awareness
    class AIModelFactory {
    public:
        // NEW: Create shared model that serves multiple factions
        static Reference<AIModelResource> CreateSharedDialogueModel(
            const String& modelPath,
            const std::vector<String>& supportedFactions
        );

        // Legacy methods (still supported)
        static Reference<AIModelResource> CreateDialogueModel(
            const String& modelPath,
            const String& factionId
        );

        static Reference<AIModelResource> CreateTerrainModel(
            const String& modelPath,
            const String& regionType
        );

        static Reference<AIModelResource> CreateBehaviorModel(
            const String& modelPath,
            const String& behaviorType
        );

    private:
        static AIModelMetadata LoadMetadataFromFile(const String& metadataPath);
        static bool ValidateModelCompatibility(const AIModelMetadata& metadata);
        static bool ValidateSharedModelCompatibility(const AIModelMetadata& metadata,
            const std::vector<String>& requiredFactions);
    };

} // namespace Angaraka::AI