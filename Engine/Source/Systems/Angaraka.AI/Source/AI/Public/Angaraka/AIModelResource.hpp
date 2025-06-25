#pragma once

#include <Angaraka/AIBase.hpp>
#include <onnxruntime_cxx_api.h>

import Angaraka.Core.Resources;

namespace Angaraka::AI {

    // Metadata for AI models
    struct AIModelMetadata {
        std::string modelType;          // "dialogue", "terrain", "behavior"
        std::string factionId;          // "ashvattha", "vaikuntha", "yuga_striders"
        std::vector<std::string> inputNames;
        std::vector<std::string> outputNames;
        std::vector<std::vector<int64_t>> inputShapes;
        std::vector<std::vector<int64_t>> outputShapes;
        float maxInferenceTimeMs;       // Performance target
        size_t maxMemoryMB;             // Memory usage limit
        std::string description;        // Human-readable description
    };

    // ONNX Model Resource for loading and managing AI models
    class AIModelResource : public Angaraka::Core::Resource {
    public:
        explicit AIModelResource(const std::string& id);
        ~AIModelResource();

        AGK_RESOURCE_TYPE_ID(AIModelResource);

        // Implement Resource interface
        bool Load(const std::string& filePath, void* context = nullptr) override;
        void Unload() override;
        size_t GetSizeInBytes(void) const override { return m_memoryUsageMB * 1024 * 1024; }

        // AI-specific methods
        std::vector<Ort::Value> RunInference(const std::vector<Ort::Value>& inputs);
        std::vector<Ort::Value> RunInferenceAsync(const std::vector<Ort::Value>& inputs);

        // Getters
        const AIModelMetadata& GetMetadata() const { return m_metadata; }
        bool IsLoaded() const { return m_session != nullptr; }
        const std::string& GetFactionId() const { return m_metadata.factionId; }
        const std::string& GetModelType() const { return m_metadata.modelType; }

        // Performance monitoring
        float GetLastInferenceTimeMs() const { return m_lastInferenceTimeMs; }
        size_t GetMemoryUsageMB() const { return m_memoryUsageMB; }

    private:
        std::unique_ptr<Ort::Session> m_session;
        std::unique_ptr<Ort::Env> m_environment;
        std::unique_ptr<Ort::SessionOptions> m_sessionOptions;
        AIModelMetadata m_metadata;

        // Performance tracking
        mutable float m_lastInferenceTimeMs{ 0.0f };
        mutable size_t m_memoryUsageMB{ 0 };

        // Thread safety for async inference
        mutable std::mutex m_inferenceMutex;

        // Helper methods
        bool LoadMetadata(const std::string& metadataPath);
        bool SetupDirectMLProvider();
        bool ValidateModelInputsOutputs();
        void UpdatePerformanceMetrics(float inferenceTimeMs, size_t memoryUsage) const;
    };

    // Factory for creating AI model resources with proper configuration
    class AIModelFactory {
    public:
        static std::shared_ptr<AIModelResource> CreateDialogueModel(
            const std::string& modelPath,
            const std::string& factionId
        );

        static std::shared_ptr<AIModelResource> CreateTerrainModel(
            const std::string& modelPath,
            const std::string& regionType
        );

        static std::shared_ptr<AIModelResource> CreateBehaviorModel(
            const std::string& modelPath,
            const std::string& behaviorType
        );

    private:
        static AIModelMetadata LoadMetadataFromFile(const std::string& metadataPath);
        static bool ValidateModelCompatibility(const AIModelMetadata& metadata);
    };

} // namespace Angaraka::AI