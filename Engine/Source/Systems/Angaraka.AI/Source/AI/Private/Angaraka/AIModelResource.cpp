// Engine/Source/Systems/Angaraka.AI/Source/AI/Modules/AIModelResource.cpp
#include <Angaraka/AIModelResource.hpp>
#include <Angaraka/Base.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <nlohmann/json.hpp>

namespace Angaraka::AI {

    AIModelResource::AIModelResource(const String& id)
        : Resource(id)
    {
        AGK_INFO("AIModelResource: Created with ID '{0}'.", id);

        // Initialize ONNX Runtime environment
        m_environment = CreateScope<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "AngarakaAI");
        m_sessionOptions = CreateScope<Ort::SessionOptions>();

        // Configure session options for optimal performance
        m_sessionOptions->SetIntraOpNumThreads(4);
        m_sessionOptions->SetInterOpNumThreads(4);
        m_sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        m_sessionOptions->EnableMemPattern();
        m_sessionOptions->EnableCpuMemArena();
        m_sessionOptions->AppendExecutionProvider("DML");
    }

    AIModelResource::~AIModelResource() {
        AGK_INFO("AIModelResource: Destructor called for ID '{0}'.", GetId());
        Unload();
    }

    bool AIModelResource::Load(const String& filePath, void* context) {
        AGK_INFO("AIModelResource: Loading AI model from '{0}'...", filePath);
        m_isLoaded = false; // Reset loaded state

        if (!std::filesystem::exists(filePath)) {
            AGK_ERROR("AIModelResource: Model file not found: '{0}'", filePath);
            return m_isLoaded;
        }

        try {
            // Load metadata first
            String metadataPath = filePath + ".meta";
            if (!LoadMetadata(metadataPath)) {
                AGK_ERROR("AIModelResource: Failed to load metadata for '{0}'", filePath);
                return m_isLoaded;
            }

            // Convert path to wide string for ONNX Runtime
            std::wstring wfilePath(filePath.begin(), filePath.end());

            auto start = std::chrono::high_resolution_clock::now();

            // Create ONNX Runtime session
            m_session = std::make_unique<Ort::Session>(*m_environment, wfilePath.c_str(), *m_sessionOptions);

            auto end = std::chrono::high_resolution_clock::now();
            F32 loadTimeMs = std::chrono::duration<F32, std::milli>(end - start).count();

            // Validate model inputs/outputs match metadata
            if (!ValidateModelInputsOutputs()) {
                AGK_ERROR("AIModelResource: Model validation failed for '{0}'", filePath);
                return m_isLoaded;
            }

            // Estimate memory usage
            m_memoryUsageMB = std::filesystem::file_size(filePath) / (1024 * 1024);

            // Setup optimal batch size for shared models
            if (IsSharedModel()) {
                m_optimalBatchSize = m_metadata.optimalBatchSize;
                m_batchProcessingEnabled = true;
                AGK_INFO("AIModelResource: Shared model detected, enabling batch processing (batch size: {0})", m_optimalBatchSize);
            }

            // Warm up the model for optimal performance
            if (IsSharedModel()) {
                WarmupModel();
                OptimizeForSharedUsage();
            }

            AGK_INFO("AIModelResource: Successfully loaded '{0}' (Type: {1}, Architecture: {2}) in {3:.2f}ms, Memory: {4}MB",
                filePath, m_metadata.modelType, m_metadata.architecture, loadTimeMs, m_memoryUsageMB);
            m_isLoaded = true;
            return m_isLoaded;
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

            // Clear faction metrics
            std::lock_guard<std::mutex> lock(m_metricsMutex);
            m_factionInferenceTimes.clear();
            m_factionInferenceCounts.clear();
            m_factionLastUsed.clear();
        }
    }

    std::vector<Ort::Value> AIModelResource::RunInference(const std::vector<Ort::Value>& inputs) {
        std::lock_guard<std::mutex> lock(m_inferenceMutex);

        if (!m_session) {
            AGK_ERROR("AIModelResource: Attempted inference on unloaded model '{0}'", GetId());
            return {};
        }

        // Validate input count matches expected inputs
        size_t expectedInputs = m_session->GetInputCount();
        if (inputs.size() != expectedInputs) {
            AGK_ERROR("AIModelResource: Input count mismatch for '{0}' - expected {1}, got {2}",
                GetId(), expectedInputs, inputs.size());
            return {};
        }

        try {
            auto start = std::chrono::high_resolution_clock::now();

            // Prepare input/output names
            std::vector<const char*> inputNames;
            std::vector<const char*> outputNames;

            // Get input names from the model
            Ort::AllocatorWithDefaultOptions allocator;
            for (size_t i = 0; i < expectedInputs; ++i) {
                auto inputName = m_session->GetInputNameAllocated(i, allocator);
                inputNames.push_back(inputName.get());
                AGK_TRACE("AIModelResource: Input {0}: {1}", i, inputName.get());
            }

            // Get output names from the model
            size_t outputCount = m_session->GetOutputCount();
            for (size_t i = 0; i < outputCount; ++i) {
                auto outputName = m_session->GetOutputNameAllocated(i, allocator);
                outputNames.push_back(outputName.get());
                AGK_TRACE("AIModelResource: Output {0}: {1}", i, outputName.get());
            }

            // Validate input tensors
            for (size_t i = 0; i < inputs.size(); ++i) {
                if (!inputs[i].IsTensor()) {
                    AGK_ERROR("AIModelResource: Input {0} is not a tensor for model '{1}'", i, GetId());
                    return {};
                }

                auto tensorInfo = inputs[i].GetTensorTypeAndShapeInfo();
                auto shape = tensorInfo.GetShape();
                AGK_TRACE("AIModelResource: Input {0} shape: [{1}]", i,
                    [&shape]() {
                        String shapeStr;
                        for (size_t j = 0; j < shape.size(); ++j) {
                            if (j > 0) shapeStr += ", ";
                            shapeStr += std::to_string(shape[j]);
                        }
                        return shapeStr;
                    }());
            }

            // Run inference
            auto outputs = m_session->Run(Ort::RunOptions{ nullptr },
                inputNames.data(), inputs.data(), inputs.size(),
                outputNames.data(), outputNames.size());

            auto end = std::chrono::high_resolution_clock::now();
            F32 inferenceTimeMs = std::chrono::duration<F32, std::milli>(end - start).count();

            UpdatePerformanceMetrics(inferenceTimeMs, m_memoryUsageMB);

            // Check if inference time exceeds target
            if (inferenceTimeMs > m_metadata.maxInferenceTimeMs) {
                AGK_WARN("AIModelResource: Inference time {0:.2f}ms exceeds target {1:.2f}ms for model '{2}'",
                    inferenceTimeMs, m_metadata.maxInferenceTimeMs, GetId());
            }

            AGK_TRACE("AIModelResource: Inference completed in {0:.2f}ms for model '{1}' with {2} outputs",
                inferenceTimeMs, GetId(), outputs.size());

            return outputs;
        }
        catch (const Ort::Exception& e) {
            AGK_ERROR("AIModelResource: Inference failed for '{0}': {1}", GetId(), e.what());

            // Log additional debugging information
            try {
                Ort::AllocatorWithDefaultOptions allocator;
                AGK_ERROR("AIModelResource: Model '{0}' expects {1} inputs, {2} outputs",
                    GetId(), m_session->GetInputCount(), m_session->GetOutputCount());

                for (size_t i = 0; i < m_session->GetInputCount(); ++i) {
                    auto inputName = m_session->GetInputNameAllocated(i, allocator);
                    auto typeInfo = m_session->GetInputTypeInfo(i);
                    auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
                    auto shape = tensorInfo.GetShape();

                    String shapeStr;
                    for (size_t j = 0; j < shape.size(); ++j) {
                        if (j > 0) shapeStr += ", ";
                        shapeStr += std::to_string(shape[j]);
                    }

                    AGK_ERROR("AIModelResource: Expected input {0}: name='{1}', shape=[{2}]",
                        i, inputName.get(), shapeStr);
                }
            }
            catch (...) {
                AGK_ERROR("AIModelResource: Failed to get model input information");
            }

            return {};
        }
    }

    std::vector<Ort::Value> AIModelResource::RunInferenceAsync(const std::vector<Ort::Value>& inputs) {
        // For now, delegate to synchronous inference
        // TODO: Implement proper async inference with thread pool
        return RunInference(inputs);
    }

    // NEW: Batch inference for multiple requests
    std::vector<std::vector<Ort::Value>> AIModelResource::RunBatchInference(const std::vector<std::vector<Ort::Value>>& batchInputs) {
        std::vector<std::vector<Ort::Value>> batchOutputs;

        if (!m_batchProcessingEnabled || batchInputs.empty()) {
            // Fallback to individual inference
            batchOutputs.reserve(batchInputs.size());
            for (const auto& inputs : batchInputs) {
                batchOutputs.push_back(RunInference(inputs));
            }
            return batchOutputs;
        }

        try {
            // TODO: Implement true batch processing by concatenating inputs
            // For now, process individually but with optimizations
            AGK_INFO("AIModelResource: Processing batch of {0} requests", batchInputs.size());

            auto start = std::chrono::high_resolution_clock::now();

            batchOutputs.reserve(batchInputs.size());
            for (const auto& inputs : batchInputs) {
                batchOutputs.push_back(RunInference(inputs));
            }

            auto end = std::chrono::high_resolution_clock::now();
            F32 totalTimeMs = std::chrono::duration<F32, std::milli>(end - start).count();

            AGK_INFO("AIModelResource: Batch inference completed in {0:.2f}ms ({1:.2f}ms per request)",
                totalTimeMs, totalTimeMs / batchInputs.size());

        }
        catch (const std::exception& e) {
            AGK_ERROR("AIModelResource: Batch inference failed: {0}", e.what());
        }

        return batchOutputs;
    }

    // NEW: Faction-aware inference with per-faction metrics
    std::vector<Ort::Value> AIModelResource::RunFactionInference(const std::vector<Ort::Value>& inputs, const String& factionId) {
        auto start = std::chrono::high_resolution_clock::now();

        // Run the actual inference
        auto outputs = RunInference(inputs);

        auto end = std::chrono::high_resolution_clock::now();
        F32 inferenceTimeMs = std::chrono::duration<F32, std::milli>(end - start).count();

        // Update faction-specific metrics
        UpdateFactionMetrics(factionId, inferenceTimeMs);

        AGK_TRACE("AIModelResource: Faction '{0}' inference completed in {1:.2f}ms", factionId, inferenceTimeMs);

        return outputs;
    }

    // NEW: Check if model supports a specific faction
    bool AIModelResource::SupportsFaction(const String& factionId) const {
        if (m_metadata.supportedFactions.empty()) {
            // If no supported factions specified, assume it supports all
            return true;
        }

        return std::find(m_metadata.supportedFactions.begin(), m_metadata.supportedFactions.end(), factionId)
            != m_metadata.supportedFactions.end();
    }

    // NEW: Get faction-specific performance metrics
    F32 AIModelResource::GetFactionInferenceTimeMs(const String& factionId) const {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        auto it = m_factionInferenceTimes.find(factionId);
        return it != m_factionInferenceTimes.end() ? it->second : 0.0f;
    }

    size_t AIModelResource::GetFactionInferenceCount(const String& factionId) const {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        auto it = m_factionInferenceCounts.find(factionId);
        return it != m_factionInferenceCounts.end() ? it->second : 0;
    }

    void AIModelResource::ResetFactionMetrics(const String& factionId) {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_factionInferenceTimes[factionId] = 0.0f;
        m_factionInferenceCounts[factionId] = 0;
        m_factionLastUsed.erase(factionId);

        AGK_INFO("AIModelResource: Reset metrics for faction '{0}'", factionId);
    }

    // UPDATED: Tensor creation methods (moved from static to instance)
    Ort::Value AIModelResource::CreateFloatTensor(const std::vector<F32>& data, const std::vector<I64>& shape) {
        try {
            Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

            // Calculate total size
            I64 totalSize = 1;
            for (I64 dim : shape) {
                totalSize *= dim;
            }

            if (static_cast<size_t>(totalSize) != data.size()) {
                AGK_ERROR("AIModelResource: Data size {0} doesn't match tensor shape size {1}", data.size(), totalSize);
                throw std::runtime_error("Data size doesn't match tensor shape");
            }

            return Ort::Value::CreateTensor<F32>(memoryInfo,
                const_cast<F32*>(data.data()), data.size(),
                shape.data(), shape.size());
        }
        catch (const std::exception& e) {
            AGK_ERROR("AIModelResource: Failed to create F32 tensor: {0}", e.what());
            throw;
        }
    }

    Ort::Value AIModelResource::CreateInt64Tensor(const std::vector<I64>& data, const std::vector<I64>& shape) {
        try {
            Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

            I64 totalSize = 1;
            for (I64 dim : shape) {
                totalSize *= dim;
            }

            if (static_cast<size_t>(totalSize) != data.size()) {
                AGK_ERROR("AIModelResource: Data size {0} doesn't match tensor shape size {1}", data.size(), totalSize);
                throw std::runtime_error("Data size doesn't match tensor shape");
            }

            return Ort::Value::CreateTensor<I64>(memoryInfo,
                const_cast<I64*>(data.data()), data.size(),
                shape.data(), shape.size());
        }
        catch (const std::exception& e) {
            AGK_ERROR("AIModelResource: Failed to create int64 tensor: {0}", e.what());
            throw;
        }
    }

    Ort::Value AIModelResource::CreateStringTensor(const std::vector<String>& data, const std::vector<I64>& shape) {
        try {
            // String tensors are more complex - convert to char* array
            std::vector<const char*> cstrings;
            cstrings.reserve(data.size());

            for (const auto& str : data) {
                cstrings.push_back(str.c_str());
            }

            Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            return Ort::Value::CreateTensor(memoryInfo, cstrings.data(), cstrings.size() * sizeof(char*),
                shape.data(), shape.size(), ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING);
        }
        catch (const std::exception& e) {
            AGK_ERROR("AIModelResource: Failed to create string tensor: {0}", e.what());
            throw;
        }
    }

    // Model introspection methods
    std::vector<String> AIModelResource::GetInputNames() const {
        if (!m_session) {
            return {};
        }

        std::vector<String> names;
        Ort::AllocatorWithDefaultOptions allocator;

        size_t inputCount = m_session->GetInputCount();
        names.reserve(inputCount);

        for (size_t i = 0; i < inputCount; ++i) {
            auto inputName = m_session->GetInputNameAllocated(i, allocator);
            names.emplace_back(inputName.get());
        }

        return names;
    }

    std::vector<String> AIModelResource::GetOutputNames() const {
        if (!m_session) {
            return {};
        }

        std::vector<String> names;
        Ort::AllocatorWithDefaultOptions allocator;

        size_t outputCount = m_session->GetOutputCount();
        names.reserve(outputCount);

        for (size_t i = 0; i < outputCount; ++i) {
            auto outputName = m_session->GetOutputNameAllocated(i, allocator);
            names.emplace_back(outputName.get());
        }

        return names;
    }

    std::vector<std::vector<I64>> AIModelResource::GetInputShapes() const {
        if (!m_session) {
            return {};
        }

        std::vector<std::vector<I64>> shapes;
        size_t inputCount = m_session->GetInputCount();
        shapes.reserve(inputCount);

        for (size_t i = 0; i < inputCount; ++i) {
            try {
                auto typeInfo = m_session->GetInputTypeInfo(i);
                auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
                auto shape = tensorInfo.GetShape();
                shapes.push_back(shape);
            }
            catch (const Ort::Exception& e) {
                AGK_WARN("AIModelResource: Failed to get input shape {0}: {1}", i, e.what());
                shapes.push_back({}); // Empty shape for unknown
            }
        }

        return shapes;
    }

    std::vector<std::vector<I64>> AIModelResource::GetOutputShapes() const {
        if (!m_session) {
            return {};
        }

        std::vector<std::vector<I64>> shapes;
        size_t outputCount = m_session->GetOutputCount();
        shapes.reserve(outputCount);

        for (size_t i = 0; i < outputCount; ++i) {
            try {
                auto typeInfo = m_session->GetOutputTypeInfo(i);
                auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
                auto shape = tensorInfo.GetShape();
                shapes.push_back(shape);
            }
            catch (const Ort::Exception& e) {
                AGK_WARN("AIModelResource: Failed to get output shape {0}: {1}", i, e.what());
                shapes.push_back({}); // Empty shape for unknown
            }
        }

        return shapes;
    }

    // NEW: Model warming for optimal performance
    bool AIModelResource::WarmupModel() {
        if (!m_session) {
            AGK_ERROR("AIModelResource: Cannot warm up unloaded model");
            return false;
        }

        try {
            AGK_INFO("AIModelResource: Warming up model '{0}'...", GetId());

            auto start = std::chrono::high_resolution_clock::now();

            // Create dummy inputs based on expected shapes
            auto inputShapes = GetInputShapes();
            std::vector<Ort::Value> dummyInputs;

            for (size_t i = 0; i < inputShapes.size(); ++i) {
                const auto& shape = inputShapes[i];

                if (shape.empty()) {
                    AGK_WARN("AIModelResource: Skipping warmup for input {0} with unknown shape", i);
                    continue;
                }

                // Calculate total elements
                I64 totalElements = 1;
                for (I64 dim : shape) {
                    if (dim > 0) totalElements *= dim;
                    else totalElements *= 1; // Handle dynamic dimensions
                }

                // Create dummy data (assume F32 tensors for warmup)
                std::vector<F32> dummyData(totalElements, 0.0f);
                auto tensor = CreateFloatTensor(dummyData, shape);
                dummyInputs.push_back(std::move(tensor));
            }

            if (!dummyInputs.empty()) {
                // Run a dummy inference to warm up the model
                auto outputs = RunInference(dummyInputs);

                auto end = std::chrono::high_resolution_clock::now();
                F32 warmupTimeMs = std::chrono::duration<F32, std::milli>(end - start).count();

                AGK_INFO("AIModelResource: Model warmup completed in {0:.2f}ms", warmupTimeMs);
                return true;
            }
            else {
                AGK_WARN("AIModelResource: Could not create dummy inputs for warmup");
                return false;
            }

        }
        catch (const std::exception& e) {
            AGK_ERROR("AIModelResource: Model warmup failed: {0}", e.what());
            return false;
        }
    }

    void AIModelResource::OptimizeForSharedUsage() {
        if (!IsSharedModel()) {
            return;
        }

        AGK_INFO("AIModelResource: Optimizing shared model '{0}' for multi-faction usage", GetId());

        // Enable optimizations specific to shared models
        try {
            // Set optimal thread configuration for shared usage
            if (m_sessionOptions) {
                // Use more threads for shared models as they handle multiple requests
                m_sessionOptions->SetIntraOpNumThreads(std::max(4, static_cast<int>(std::thread::hardware_concurrency() / 2)));
                m_sessionOptions->SetInterOpNumThreads(std::max(2, static_cast<int>(std::thread::hardware_concurrency() / 4)));
            }

            // Initialize faction tracking
            for (const auto& factionId : m_metadata.supportedFactions) {
                std::lock_guard<std::mutex> lock(m_metricsMutex);
                m_factionInferenceTimes[factionId] = 0.0f;
                m_factionInferenceCounts[factionId] = 0;
                AGK_TRACE("AIModelResource: Initialized tracking for faction '{0}'", factionId);
            }

        }
        catch (const std::exception& e) {
            AGK_WARN("AIModelResource: Failed to optimize for shared usage: {0}", e.what());
        }
    }

    // ===== HELPER METHODS =====

    bool AIModelResource::LoadMetadata(const String& metadataPath) {
        try {
            if (!std::filesystem::exists(metadataPath)) {
                AGK_WARN("AIModelResource: Metadata file not found: '{0}', using defaults", metadataPath);

                // Set default metadata for shared models
                m_metadata.modelType = "dialogue";
                m_metadata.architecture = "shared_multi_faction";
                m_metadata.maxInferenceTimeMs = 100.0f;
                m_metadata.maxMemoryMB = 1024;
                m_metadata.optimalBatchSize = 4;
                m_metadata.supportsAsyncInference = true;
                m_metadata.description = "Shared AI model (no metadata available)";
                m_metadata.version = "1.0";
                m_metadata.supportedFactions = { "ashvattha", "vaikuntha", "yuga_striders", "shroud_mantra" };
                return true;
            }

            std::ifstream file(metadataPath);
            nlohmann::json metaJson;
            file >> metaJson;

            m_metadata.modelType = metaJson.value("modelType", "dialogue");
            m_metadata.architecture = metaJson.value("architecture", "single_faction");
            m_metadata.maxInferenceTimeMs = metaJson.value("maxInferenceTimeMs", 100.0f);
            m_metadata.maxMemoryMB = metaJson.value("maxMemoryMB", 1024);
            m_metadata.optimalBatchSize = metaJson.value("optimalBatchSize", 1);
            m_metadata.supportsAsyncInference = metaJson.value("supportsAsyncInference", false);
            m_metadata.description = metaJson.value("description", "");
            m_metadata.version = metaJson.value("version", "1.0");

            // Parse supported factions
            if (metaJson.contains("supportedFactions") && metaJson["supportedFactions"].is_array()) {
                for (const auto& faction : metaJson["supportedFactions"]) {
                    if (faction.is_string()) {
                        m_metadata.supportedFactions.push_back(faction.get<String>());
                    }
                }
            }

            if (metaJson.contains("inputNames")) {
                m_metadata.inputNames = metaJson["inputNames"].get<std::vector<String>>();
            }
            if (metaJson.contains("outputNames")) {
                m_metadata.outputNames = metaJson["outputNames"].get<std::vector<String>>();
            }

            AGK_INFO("AIModelResource: Loaded metadata - Type: {0}, Architecture: {1}, Supported Factions: {2}",
                m_metadata.modelType, m_metadata.architecture, m_metadata.supportedFactions.size());

            return true;
        }
        catch (const std::exception& e) {
            AGK_ERROR("AIModelResource: Failed to load metadata from '{0}': {1}", metadataPath, e.what());
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
                    m_metadata.inputNames.push_back(String(inputName.get()));
                }
            }

            if (m_metadata.outputNames.empty()) {
                for (size_t i = 0; i < numOutputs; ++i) {
                    auto outputName = m_session->GetOutputNameAllocated(i, allocator);
                    m_metadata.outputNames.push_back(String(outputName.get()));
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

    void AIModelResource::UpdatePerformanceMetrics(F32 inferenceTimeMs, size_t memoryUsage) const {
        m_lastInferenceTimeMs = inferenceTimeMs;
        m_memoryUsageMB = memoryUsage;
    }

    void AIModelResource::UpdateFactionMetrics(const String& factionId, F32 inferenceTimeMs) const {
        std::lock_guard<std::mutex> lock(m_metricsMutex);

        // Update running average of inference times
        auto& currentTime = m_factionInferenceTimes[factionId];
        auto& count = m_factionInferenceCounts[factionId];

        currentTime = (currentTime * count + inferenceTimeMs) / (count + 1);
        count++;

        // Update last used timestamp
        m_factionLastUsed[factionId] = std::chrono::steady_clock::now();

        AGK_TRACE("AIModelResource: Updated metrics for faction '{0}': avg={1:.2f}ms, count={2}",
            factionId, currentTime, count);

        // Periodically clean up old metrics
        if (count % 100 == 0) { // Every 100 inferences
            CleanupOldFactionMetrics();
        }
    }

    void AIModelResource::CleanupOldFactionMetrics() const {
        auto now = std::chrono::steady_clock::now();
        std::vector<String> factionsToClean;

        // Find factions that haven't been used in a while
        for (const auto& [factionId, lastUsed] : m_factionLastUsed) {
            auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - lastUsed);
            if (elapsed.count() > 30) { // 30 minutes of inactivity
                factionsToClean.push_back(factionId);
            }
        }

        // Clean up old metrics
        for (const auto& factionId : factionsToClean) {
            m_factionInferenceTimes.erase(factionId);
            m_factionInferenceCounts.erase(factionId);
            m_factionLastUsed.erase(factionId);
            AGK_TRACE("AIModelResource: Cleaned up old metrics for faction '{0}'", factionId);
        }
    }

    // ===== FACTORY METHODS =====

    Reference<AIModelResource> AIModelFactory::CreateSharedDialogueModel(
        const String& modelPath, const std::vector<String>& supportedFactions) {

        auto model = std::make_shared<AIModelResource>("shared_dialogue_model");

        if (!model->Load(modelPath)) {
            AGK_ERROR("AIModelFactory: Failed to load shared dialogue model from '{0}'", modelPath);
            return nullptr;
        }

        // Validate that the model supports the required factions
        for (const auto& factionId : supportedFactions) {
            if (!model->SupportsFaction(factionId)) {
                AGK_WARN("AIModelFactory: Model may not fully support faction '{0}'", factionId);
            }
        }

        AGK_INFO("AIModelFactory: Created shared dialogue model supporting {0} factions", supportedFactions.size());
        return model;
    }

    // Legacy factory methods remain unchanged
    Reference<AIModelResource> AIModelFactory::CreateDialogueModel(
        const String& modelPath, const String& factionId) {

        auto model = std::make_shared<AIModelResource>(factionId + "_dialogue");

        if (model->Load(modelPath)) {
            return model;
        }

        return nullptr;
    }

    Reference<AIModelResource> AIModelFactory::CreateTerrainModel(
        const String& modelPath, const String& regionType) {

        auto model = std::make_shared<AIModelResource>(regionType + "_terrain");

        if (model->Load(modelPath)) {
            return model;
        }

        return nullptr;
    }

    Reference<AIModelResource> AIModelFactory::CreateBehaviorModel(
        const String& modelPath, const String& behaviorType) {

        auto model = std::make_shared<AIModelResource>(behaviorType + "_behavior");

        if (model->Load(modelPath)) {
            return model;
        }

        return nullptr;
    }

} // namespace Angaraka::AI