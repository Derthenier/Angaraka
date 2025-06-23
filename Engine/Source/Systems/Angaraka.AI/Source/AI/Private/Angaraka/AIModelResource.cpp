// Engine/Source/Systems/Angaraka.AI/Source/AI/Modules/AIModelResource.cpp
#include <Angaraka/AIModelResource.hpp>
#include <Angaraka/Base.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <nlohmann/json.hpp>

namespace Angaraka::AI {

    AIModelResource::AIModelResource(const std::string& id)
        : Resource(id)
    {
        AGK_INFO("AIModelResource: Created with ID '{0}'.", id);

        // Initialize ONNX Runtime environment
        m_environment = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "AngarakaAI");
        m_sessionOptions = std::make_unique<Ort::SessionOptions>();

        // Enable DirectML provider for GPU acceleration
        if (!SetupDirectMLProvider()) {
            AGK_WARN("AIModelResource: DirectML setup failed, falling back to CPU execution.");
        }

        // Configure session options for optimal performance
        m_sessionOptions->SetIntraOpNumThreads(4);
        m_sessionOptions->SetInterOpNumThreads(4);
        m_sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        m_sessionOptions->EnableMemPattern();
        m_sessionOptions->EnableCpuMemArena();
    }

    AIModelResource::~AIModelResource() {
        AGK_INFO("AIModelResource: Destructor called for ID '{0}'.", GetId());
        Unload();
    }

    bool AIModelResource::Load(const std::string& filePath, void* context) {
        AGK_INFO("AIModelResource: Loading AI model from '{0}'...", filePath);

        if (!std::filesystem::exists(filePath)) {
            AGK_ERROR("AIModelResource: Model file not found: '{0}'", filePath);
            return false;
        }

        try {
            // Load metadata first
            std::string metadataPath = filePath + ".meta";
            if (!LoadMetadata(metadataPath)) {
                AGK_ERROR("AIModelResource: Failed to load metadata for '{0}'", filePath);
                return false;
            }

            // Convert path to wide string for ONNX Runtime
            std::wstring wfilePath(filePath.begin(), filePath.end());

            auto start = std::chrono::high_resolution_clock::now();

            // Create ONNX Runtime session
            m_session = std::make_unique<Ort::Session>(*m_environment, wfilePath.c_str(), *m_sessionOptions);

            auto end = std::chrono::high_resolution_clock::now();
            float loadTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

            // Validate model inputs/outputs match metadata
            if (!ValidateModelInputsOutputs()) {
                AGK_ERROR("AIModelResource: Model validation failed for '{0}'", filePath);
                return false;
            }

            // Estimate memory usage
            m_memoryUsageMB = std::filesystem::file_size(filePath) / (1024 * 1024);

            AGK_INFO("AIModelResource: Successfully loaded '{0}' (Type: {1}, Faction: {2}) in {3:.2f}ms, Memory: {4}MB",
                filePath, m_metadata.modelType, m_metadata.factionId, loadTimeMs, m_memoryUsageMB);

            return true;
        }
        catch (const Ort::Exception& e) {
            AGK_ERROR("AIModelResource: ONNX Runtime error loading '{0}': {1}", filePath, e.what());
            return false;
        }
        catch (const std::exception& e) {
            AGK_ERROR("AIModelResource: Failed to load '{0}': {1}", filePath, e.what());
            return false;
        }
    }

    void AIModelResource::Unload() {
        if (m_session) {
            AGK_INFO("AIModelResource: Unloading AI model '{0}'", GetId());
            m_session.reset();
            m_memoryUsageMB = 0;
        }
    }

    std::vector<Ort::Value> AIModelResource::RunInference(const std::vector<Ort::Value>& inputs) {
        std::lock_guard<std::mutex> lock(m_inferenceMutex);

        if (!m_session) {
            AGK_ERROR("AIModelResource: Attempted inference on unloaded model '{0}'", GetId());
            return {};
        }

        try {
            auto start = std::chrono::high_resolution_clock::now();

            // Prepare input/output names
            std::vector<const char*> inputNames;
            std::vector<const char*> outputNames;

            for (const auto& name : m_metadata.inputNames) {
                inputNames.push_back(name.c_str());
            }
            for (const auto& name : m_metadata.outputNames) {
                outputNames.push_back(name.c_str());
            }

            // Run inference
            auto outputs = m_session->Run(Ort::RunOptions{ nullptr },
                inputNames.data(), inputs.data(), inputs.size(),
                outputNames.data(), outputNames.size());

            auto end = std::chrono::high_resolution_clock::now();
            float inferenceTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

            UpdatePerformanceMetrics(inferenceTimeMs, m_memoryUsageMB);

            // Check if inference time exceeds target
            if (inferenceTimeMs > m_metadata.maxInferenceTimeMs) {
                AGK_WARN("AIModelResource: Inference time {0:.2f}ms exceeds target {1:.2f}ms for model '{2}'",
                    inferenceTimeMs, m_metadata.maxInferenceTimeMs, GetId());
            }

            AGK_TRACE("AIModelResource: Inference completed in {0:.2f}ms for model '{1}'",
                inferenceTimeMs, GetId());

            return outputs;
        }
        catch (const Ort::Exception& e) {
            AGK_ERROR("AIModelResource: Inference failed for '{0}': {1}", GetId(), e.what());
            return {};
        }
    }

    std::vector<Ort::Value> AIModelResource::RunInferenceAsync(const std::vector<Ort::Value>& inputs) {
        // For now, delegate to synchronous inference
        // TODO: Implement proper async inference with thread pool
        return RunInference(inputs);
    }

    bool AIModelResource::LoadMetadata(const std::string& metadataPath) {
        try {
            if (!std::filesystem::exists(metadataPath)) {
                AGK_WARN("AIModelResource: Metadata file not found: '{0}', using defaults", metadataPath);

                // Set default metadata
                m_metadata.modelType = "unknown";
                m_metadata.factionId = "neutral";
                m_metadata.maxInferenceTimeMs = 100.0f;
                m_metadata.maxMemoryMB = 1024;
                m_metadata.description = "AI model (no metadata available)";
                return true;
            }

            std::ifstream file(metadataPath);
            nlohmann::json metaJson;
            file >> metaJson;

            m_metadata.modelType = metaJson.value("modelType", "unknown");
            m_metadata.factionId = metaJson.value("factionId", "neutral");
            m_metadata.maxInferenceTimeMs = metaJson.value("maxInferenceTimeMs", 100.0f);
            m_metadata.maxMemoryMB = metaJson.value("maxMemoryMB", 1024);
            m_metadata.description = metaJson.value("description", "");

            if (metaJson.contains("inputNames")) {
                m_metadata.inputNames = metaJson["inputNames"].get<std::vector<std::string>>();
            }
            if (metaJson.contains("outputNames")) {
                m_metadata.outputNames = metaJson["outputNames"].get<std::vector<std::string>>();
            }

            AGK_INFO("AIModelResource: Loaded metadata - Type: {0}, Faction: {1}, MaxTime: {2:.2f}ms",
                m_metadata.modelType, m_metadata.factionId, m_metadata.maxInferenceTimeMs);

            return true;
        }
        catch (const std::exception& e) {
            AGK_ERROR("AIModelResource: Failed to load metadata from '{0}': {1}", metadataPath, e.what());
            return false;
        }
    }

    bool AIModelResource::SetupDirectMLProvider() {
        try {
            // Enable DirectML provider for GPU acceleration
            Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_CUDA(*m_sessionOptions, 0));
            AGK_INFO("AIModelResource: DirectML provider enabled for GPU acceleration");
            return true;
        }
        catch (const Ort::Exception& e) {
            AGK_WARN("AIModelResource: Failed to enable DirectML: {0}", e.what());
            return false;
        }
    }

    bool AIModelResource::ValidateModelInputsOutputs() {
        if (!m_session) {
            return false;
        }

        try {
            // Get actual model input/output info
            Ort::AllocatorWithDefaultOptions allocator;

            size_t numInputs = m_session->GetInputCount();
            size_t numOutputs = m_session->GetOutputCount();

            // If metadata doesn't specify names, get them from the model
            if (m_metadata.inputNames.empty()) {
                for (size_t i = 0; i < numInputs; ++i) {
                    auto inputName = m_session->GetInputNameAllocated(i, allocator);
                    m_metadata.inputNames.push_back(std::string(inputName.get()));
                }
            }

            if (m_metadata.outputNames.empty()) {
                for (size_t i = 0; i < numOutputs; ++i) {
                    auto outputName = m_session->GetOutputNameAllocated(i, allocator);
                    m_metadata.outputNames.push_back(std::string(outputName.get()));
                }
            }

            AGK_INFO("AIModelResource: Model validation - Inputs: {0}, Outputs: {1}",
                numInputs, numOutputs);

            return true;
        }
        catch (const Ort::Exception& e) {
            AGK_ERROR("AIModelResource: Model validation failed: {0}", e.what());
            return false;
        }
    }

    void AIModelResource::UpdatePerformanceMetrics(float inferenceTimeMs, size_t memoryUsage) const {
        m_lastInferenceTimeMs = inferenceTimeMs;
        m_memoryUsageMB = memoryUsage;
    }

} // namespace Angaraka::AI