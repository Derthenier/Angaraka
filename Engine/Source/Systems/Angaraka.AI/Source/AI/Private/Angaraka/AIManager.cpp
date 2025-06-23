// Engine/Source/Systems/Angaraka.AI/Source/AI/Modules/AIManager.cpp
#include <Angaraka/AIManager.hpp>
#include <Angaraka/Base.hpp>
#include <algorithm>
#include <filesystem>

import Angaraka.Core.Resources;

namespace Angaraka::AI {

    AIManager::AIManager(const AISystemConfig& config)
        : m_config(config)
        , m_activeFaction(config.defaultFaction)
        , m_lastMetricsUpdate(std::chrono::steady_clock::now())
    {
        AGK_INFO("AIManager: Initializing with GPU acceleration: {0}, Max VRAM: {1}MB",
            m_config.enableGPUAcceleration, m_config.maxVRAMUsageMB);
    }

    AIManager::~AIManager() {
        AGK_INFO("AIManager: Destructor called");
        Shutdown();
    }

    bool AIManager::Initialize(Reference<Angaraka::Core::CachedResourceManager>& cachedManager) {
        AGK_INFO("AIManager: Starting initialization...");

        try {
            // Get resource manager instance
            m_resourceManager = cachedManager;
            if (!m_resourceManager) {
                AGK_ERROR("AIManager: Failed to get ResourceManager instance");
                return false;
            }

            // Start background worker threads
            m_shutdownRequested = false;
            for (size_t i = 0; i < m_config.backgroundThreadCount; ++i) {
                m_backgroundThreads.emplace_back([this]() { ProcessBackgroundTasks(); });
                AGK_TRACE("AIManager: Started background thread {0}", i);
            }

            // Initialize performance monitoring
            m_performanceMetrics = AIPerformanceMetrics{};
            m_profilingEnabled = m_config.enablePerformanceMonitoring;

            AGK_INFO("AIManager: Successfully initialized with {0} background threads",
                m_config.backgroundThreadCount);
            return true;
        }
        catch (const std::exception& e) {
            AGK_ERROR("AIManager: Initialization failed: {0}", e.what());
            return false;
        }
    }

    void AIManager::Shutdown() {
        AGK_INFO("AIManager: Starting shutdown...");

        // Signal shutdown to background threads
        m_shutdownRequested = true;
        m_taskCondition.notify_all();

        // Wait for all background threads to finish
        for (auto& thread : m_backgroundThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        m_backgroundThreads.clear();

        // Unload all models
        m_loadedModels.clear();
        m_factionModels.clear();
        m_currentVRAMUsage = 0;

        AGK_INFO("AIManager: Shutdown complete");
    }

    void AIManager::Update(F32 deltaTime) {
        // Update performance metrics periodically
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastMetricsUpdate);

        if (elapsed.count() > 1000) { // Update every second
            UpdatePerformanceMetrics();
            m_lastMetricsUpdate = now;
        }

        // Check for model file changes if hot-swapping is enabled
        if (m_hotSwappingEnabled) {
            CheckForModelFileChanges();
        }

        // Clean up unused models periodically
        static F32 cleanupTimer = 0.0f;
        cleanupTimer += deltaTime;
        if (cleanupTimer > 30.0f) { // Clean up every 30 seconds
            CleanupUnusedModels();
            cleanupTimer = 0.0f;
        }
    }

    bool AIManager::LoadDialogueModel(const String& factionId, const String& modelPath) {
        String modelId = GenerateModelId(factionId, "dialogue");
        AGK_INFO("AIManager: Loading dialogue model for faction '{0}' from '{1}'", factionId, modelPath);

        return LoadModelInternal(modelId, modelPath, factionId);
    }

    bool AIManager::LoadTerrainModel(const String& regionType, const String& modelPath) {
        String modelId = GenerateModelId(regionType, "terrain");
        AGK_INFO("AIManager: Loading terrain model for region '{0}' from '{1}'", regionType, modelPath);

        return LoadModelInternal(modelId, modelPath, regionType);
    }

    bool AIManager::LoadBehaviorModel(const String& behaviorType, const String& modelPath) {
        String modelId = GenerateModelId(behaviorType, "behavior");
        AGK_INFO("AIManager: Loading behavior model for type '{0}' from '{1}'", behaviorType, modelPath);

        return LoadModelInternal(modelId, modelPath, behaviorType);
    }

    void AIManager::UnloadModel(const String& modelId) {
        auto it = m_loadedModels.find(modelId);
        if (it != m_loadedModels.end()) {
            AGK_INFO("AIManager: Unloading model '{0}'", modelId);

            // Update VRAM usage
            m_currentVRAMUsage -= it->second->GetMemoryUsageMB();

            // Remove from faction mapping
            for (auto& [factionId, modelIds] : m_factionModels) {
                auto modelIt = std::find(modelIds.begin(), modelIds.end(), modelId);
                if (modelIt != modelIds.end()) {
                    modelIds.erase(modelIt);
                    break;
                }
            }

            // Remove the model
            m_loadedModels.erase(it);

            AGK_INFO("AIManager: Model '{0}' unloaded, VRAM usage: {1}MB", modelId, m_currentVRAMUsage);
        }
    }

    void AIManager::UnloadFactionModels(const String& factionId) {
        auto it = m_factionModels.find(factionId);
        if (it != m_factionModels.end()) {
            AGK_INFO("AIManager: Unloading all models for faction '{0}'", factionId);

            // Unload each model
            for (const auto& modelId : it->second) {
                UnloadModel(modelId);
            }

            // Remove faction entry
            m_factionModels.erase(it);
        }
    }

    std::future<DialogueResponse> AIManager::GenerateDialogue(const DialogueRequest& request) {
        return std::async(std::launch::async, [this, request]() {
            return GenerateDialogueSync(request);
            });
    }

    std::future<TerrainResponse> AIManager::GenerateTerrain(const TerrainRequest& request) {
        return std::async(std::launch::async, [this, request]() {
            return GenerateTerrainSync(request);
        });
    }

    std::future<BehaviorResponse> AIManager::EvaluateBehavior(const BehaviorRequest& request) {
        return std::async(std::launch::async, [this, request]() {
            return EvaluateBehaviorSync(request);
        });
    }

    DialogueResponse AIManager::GenerateDialogueSync(const DialogueRequest& request) {
        DialogueResponse response;
        response.success = false;

        auto start = std::chrono::high_resolution_clock::now();

        try {
            // Get dialogue model for the faction
            auto model = GetModelForFaction(request.factionId, "dialogue");
            if (!model) {
                AGK_ERROR("AIManager: No dialogue model found for faction '{0}'", request.factionId);
                return response;
            }

            // Prepare input tensors (this would be more complex in real implementation)
            std::vector<Ort::Value> inputs;
            // TODO: Convert request data to ONNX tensors

            // Run inference
            auto outputs = model->RunInference(inputs);
            if (outputs.empty()) {
                AGK_ERROR("AIManager: Dialogue inference failed for faction '{0}'", request.factionId);
                return response;
            }

            // Process outputs (simplified for now)
            // TODO: Convert ONNX outputs to DialogueResponse
            response.response = "Philosophical response from " + request.factionId;
            response.emotionalTone = "contemplative";
            response.confidence = 0.85f;
            response.success = true;

        }
        catch (const std::exception& e) {
            AGK_ERROR("AIManager: Exception in dialogue generation: {0}", e.what());
        }

        auto end = std::chrono::high_resolution_clock::now();
        response.inferenceTimeMs = std::chrono::duration<F32, std::milli>(end - start).count();

        return response;
    }

    BehaviorResponse AIManager::EvaluateBehaviorSync(const BehaviorRequest& request) {
        BehaviorResponse response;
        response.success = false;

        auto start = std::chrono::high_resolution_clock::now();

        try {
            // Get behavior model for the faction
            auto model = GetModelForFaction(request.factionId, "behavior");
            if (!model) {
                AGK_ERROR("AIManager: No behavior model found for faction '{0}'", request.factionId);
                return response;
            }

            // Prepare input tensors
            std::vector<Ort::Value> inputs;
            // TODO: Convert request data to ONNX tensors

            // Run inference
            auto outputs = model->RunInference(inputs);
            if (outputs.empty()) {
                AGK_ERROR("AIManager: Behavior inference failed for faction '{0}'", request.factionId);
                return response;
            }

            // Process outputs
            // TODO: Convert ONNX outputs to BehaviorResponse
            if (!request.availableActions.empty()) {
                response.selectedAction = request.availableActions[0]; // Simplified
                response.reasoning = "Action selected based on " + request.factionId + " philosophy";
                response.success = true;
            }

        }
        catch (const std::exception& e) {
            AGK_ERROR("AIManager: Exception in behavior evaluation: {0}", e.what());
        }

        auto end = std::chrono::high_resolution_clock::now();
        response.inferenceTimeMs = std::chrono::duration<F32, std::milli>(end - start).count();

        return response;
    }

    TerrainResponse AIManager::GenerateTerrainSync(const TerrainRequest& request) {
        TerrainResponse response;
        response.success = false;

        auto start = std::chrono::high_resolution_clock::now();

        try {
            // Get terrain model for the region
            auto model = GetModelForFaction(request.regionType, "terrain");
            if (!model) {
                AGK_ERROR("AIManager: No terrain model found for region '{0}'", request.regionType);
                return response;
            }

            // Prepare input tensors
            std::vector<Ort::Value> inputs;
            // TODO: Convert request data to ONNX tensors including Vector3 position

            // Run inference
            auto outputs = model->RunInference(inputs);
            if (outputs.empty()) {
                AGK_ERROR("AIManager: Terrain inference failed for region '{0}'", request.regionType);
                return response;
            }

            // Process outputs - generate terrain based on faction influence
            // TODO: Convert ONNX outputs to TerrainResponse

            // Generate some sample data using our math library
            int32_t mapSize = static_cast<int32_t>(request.radius * 2);
            response.heightMap.resize(mapSize * mapSize);

            // Simple procedural generation for now
            for (int32_t y = 0; y < mapSize; ++y) {
                for (int32_t x = 0; x < mapSize; ++x) {
                    Vector3 worldPos = request.centerPosition + Vector3(
                        static_cast<F32>(x - mapSize / 2),
                        0.0f,
                        static_cast<F32>(y - mapSize / 2)
                    );

                    // Sample height based on distance from center
                    F32 distance = worldPos.DistanceTo(request.centerPosition);
                    F32 height = std::max(0.0f, 1.0f - (distance / request.radius));

                    response.heightMap[y * mapSize + x] = height * request.philosophicalResonance;
                }
            }

            // Add some landmark positions
            response.landmarkPositions.push_back(request.centerPosition + Vector3(10.0f, 0.0f, 0.0f));
            response.landmarkPositions.push_back(request.centerPosition + Vector3(-10.0f, 0.0f, 10.0f));
            response.landmarkTypes.push_back("PhilosophicalShrine");
            response.landmarkTypes.push_back("AncientRuin");

            response.success = true;

        }
        catch (const std::exception& e) {
            AGK_ERROR("AIManager: Exception in terrain generation: {0}", e.what());
        }

        auto end = std::chrono::high_resolution_clock::now();
        response.inferenceTimeMs = std::chrono::duration<F32, std::milli>(end - start).count();

        return response;
    }

    bool AIManager::SwitchActiveFaction(const String& factionId) {
        if (IsFactionLoaded(factionId)) {
            m_activeFaction = factionId;
            AGK_INFO("AIManager: Switched to faction '{0}'", factionId);
            return true;
        }

        AGK_WARN("AIManager: Cannot switch to faction '{0}' - not loaded", factionId);
        return false;
    }

    std::vector<String> AIManager::GetLoadedFactions() const {
        std::vector<String> factions;
        factions.reserve(m_factionModels.size());

        for (const auto& [factionId, modelIds] : m_factionModels) {
            if (!modelIds.empty()) {
                factions.push_back(factionId);
            }
        }

        return factions;
    }

    bool AIManager::IsFactionLoaded(const String& factionId) const {
        auto it = m_factionModels.find(factionId);
        return it != m_factionModels.end() && !it->second.empty();
    }

    F32 AIManager::GetVRAMUsagePercent() const {
        return static_cast<F32>(m_currentVRAMUsage) / static_cast<F32>(m_config.maxVRAMUsageMB) * 100.0f;
    }

    void AIManager::OptimizeMemoryUsage() {
        AGK_INFO("AIManager: Optimizing memory usage - current: {0}MB/{1}MB ({2:.1f}%)",
            m_currentVRAMUsage, m_config.maxVRAMUsageMB, GetVRAMUsagePercent());

        // If over memory limit, unload least recently used models
        if (m_currentVRAMUsage > m_config.maxVRAMUsageMB) {
            CleanupUnusedModels();
        }
    }

    bool AIManager::HotSwapModel(const String& modelId, const String& newModelPath) {
        if (!m_hotSwappingEnabled) {
            AGK_WARN("AIManager: Hot-swapping is disabled");
            return false;
        }

        auto it = m_loadedModels.find(modelId);
        if (it == m_loadedModels.end()) {
            AGK_WARN("AIManager: Cannot hot-swap model '{0}' - not currently loaded", modelId);
            return false;
        }

        AGK_INFO("AIManager: Hot-swapping model '{0}' with '{1}'", modelId, newModelPath);

        // Create new model resource
        auto newModel = std::make_shared<AIModelResource>(modelId + "_hotswap");
        if (!newModel->Load(newModelPath)) {
            AGK_ERROR("AIManager: Failed to load new model for hot-swap: '{0}'", newModelPath);
            return false;
        }

        // Replace the old model
        size_t oldMemory = it->second->GetMemoryUsageMB();
        size_t newMemory = newModel->GetMemoryUsageMB();

        it->second = newModel;
        m_currentVRAMUsage = m_currentVRAMUsage - oldMemory + newMemory;

        AGK_INFO("AIManager: Hot-swap successful for '{0}' - memory change: {1}MB -> {2}MB",
            modelId, oldMemory, newMemory);

        return true;
    }

    Reference<AIModelResource> AIManager::GetModelForFaction(const String& factionId, const String& modelType) {
        String modelId = GenerateModelId(factionId, modelType);

        auto it = m_loadedModels.find(modelId);
        if (it != m_loadedModels.end()) {
            return it->second;
        }

        // Try fallback to default faction if specific faction model not found
        if (factionId != m_config.defaultFaction) {
            String fallbackId = GenerateModelId(m_config.defaultFaction, modelType);
            auto fallbackIt = m_loadedModels.find(fallbackId);
            if (fallbackIt != m_loadedModels.end()) {
                AGK_WARN("AIManager: Using fallback model '{0}' for faction '{1}' type '{2}'",
                    fallbackId, factionId, modelType);
                return fallbackIt->second;
            }
        }

        return nullptr;
    }

    String AIManager::GenerateModelId(const String& factionId, const String& modelType) {
        return factionId + "_" + modelType;
    }

    bool AIManager::ValidateModelCompatibility(const Reference<AIModelResource>& model) {
        if (!model || !model->IsLoaded()) {
            return false;
        }

        const auto& metadata = model->GetMetadata();

        // Check if model type is supported
        static const std::vector<String> supportedTypes = { "dialogue", "terrain", "behavior" };
        bool typeSupported = std::find(supportedTypes.begin(), supportedTypes.end(), metadata.modelType) != supportedTypes.end();

        if (!typeSupported) {
            AGK_WARN("AIManager: Unsupported model type: '{0}'", metadata.modelType);
            return false;
        }

        // Check performance requirements
        if (metadata.maxInferenceTimeMs > m_config.dialogueTimeoutMs && metadata.modelType == "dialogue") {
            AGK_WARN("AIManager: Dialogue model exceeds time limit: {0}ms > {1}ms",
                metadata.maxInferenceTimeMs, m_config.dialogueTimeoutMs);
        }

        if (metadata.maxInferenceTimeMs > m_config.terrainTimeoutMs && metadata.modelType == "terrain") {
            AGK_WARN("AIManager: Terrain model exceeds time limit: {0}ms > {1}ms",
                metadata.maxInferenceTimeMs, m_config.terrainTimeoutMs);
        }

        // Check memory requirements
        if (metadata.maxMemoryMB + m_currentVRAMUsage > m_config.maxVRAMUsageMB) {
            AGK_WARN("AIManager: Model would exceed VRAM limit: {0}MB + {1}MB > {2}MB",
                metadata.maxMemoryMB, m_currentVRAMUsage, m_config.maxVRAMUsageMB);
            return false;
        }

        return true;
    }

    void AIManager::UpdatePerformanceMetrics() {
        if (!m_profilingEnabled) {
            return;
        }

        // Calculate average inference times from all loaded models
        F32 totalDialogueTime = 0.0f;
        F32 totalTerrainTime = 0.0f;
        size_t dialogueCount = 0;
        size_t terrainCount = 0;
        size_t totalMemory = 0;

        for (const auto& [modelId, model] : m_loadedModels) {
            const auto& metadata = model->GetMetadata();
            totalMemory += model->GetMemoryUsageMB();

            if (metadata.modelType == "dialogue") {
                totalDialogueTime += model->GetLastInferenceTimeMs();
                dialogueCount++;
            }
            else if (metadata.modelType == "terrain") {
                totalTerrainTime += model->GetLastInferenceTimeMs();
                terrainCount++;
            }
        }

        // Update metrics
        m_performanceMetrics.averageDialogueTimeMs = dialogueCount > 0 ? totalDialogueTime / dialogueCount : 0.0f;
        m_performanceMetrics.averageTerrainTimeMs = terrainCount > 0 ? totalTerrainTime / terrainCount : 0.0f;
        m_performanceMetrics.totalVRAMUsageMB = totalMemory;
        m_currentVRAMUsage = totalMemory;

        // Estimate GPU utilization (simplified)
        F32 vramPercent = GetVRAMUsagePercent();
        m_performanceMetrics.gpuUtilizationPercent = std::min(100.0f, vramPercent * 1.2f); // Rough estimate

        AGK_TRACE("AIManager: Performance - Dialogue: {0:.2f}ms, Terrain: {1:.2f}ms, VRAM: {2}MB ({3:.1f}%)",
            m_performanceMetrics.averageDialogueTimeMs,
            m_performanceMetrics.averageTerrainTimeMs,
            m_performanceMetrics.totalVRAMUsageMB,
            vramPercent);
    }

    void AIManager::ProcessBackgroundTasks() {
        AGK_TRACE("AIManager: Background thread started");

        while (!m_shutdownRequested) {
            std::function<void()> task;

            // Wait for a task or shutdown signal
            {
                std::unique_lock<std::mutex> lock(m_taskQueueMutex);
                m_taskCondition.wait(lock, [this] {
                    return !m_taskQueue.empty() || m_shutdownRequested;
                    });

                if (m_shutdownRequested) {
                    break;
                }

                if (!m_taskQueue.empty()) {
                    task = std::move(m_taskQueue.front());
                    m_taskQueue.pop();
                }
            }

            // Execute the task
            if (task) {
                try {
                    task();
                }
                catch (const std::exception& e) {
                    AGK_ERROR("AIManager: Exception in background task: {0}", e.what());
                }
            }
        }

        AGK_TRACE("AIManager: Background thread shutting down");
    }

    void AIManager::CheckForModelFileChanges() {
        for (const auto& [modelId, model] : m_loadedModels) {
            // This would require storing file paths and checking modification times
            // Implementation would depend on your file monitoring requirements
            // For now, this is a placeholder for development hot-swapping
        }
    }

    void AIManager::CleanupUnusedModels() {
        // Remove models that haven't been used recently or are over memory limit
        std::vector<String> modelsToUnload;

        for (const auto& [modelId, model] : m_loadedModels) {
            // Simple cleanup strategy: unload models that exceed our VRAM budget
            if (m_currentVRAMUsage > m_config.maxVRAMUsageMB) {
                // Could implement LRU cache here, for now just warn
                AGK_TRACE("AIManager: Model '{0}' candidate for cleanup (memory: {1}MB)",
                    modelId, model->GetMemoryUsageMB());
            }
        }

        // In a real implementation, you'd implement proper LRU eviction here
        if (m_currentVRAMUsage > m_config.maxVRAMUsageMB) {
            AGK_WARN("AIManager: VRAM usage {0}MB exceeds limit {1}MB - consider unloading unused models",
                m_currentVRAMUsage, m_config.maxVRAMUsageMB);
        }
    }

    bool AIManager::LoadModelInternal(const String& modelId, const String& modelPath, const String& factionId) {
        // Check if model is already loaded
        if (m_loadedModels.find(modelId) != m_loadedModels.end()) {
            AGK_INFO("AIManager: Model '{0}' already loaded", modelId);
            return true;
        }

        // Create and load the model resource
        auto model = std::make_shared<AIModelResource>(modelId);
        if (!model->Load(modelPath)) {
            AGK_ERROR("AIManager: Failed to load model '{0}' from '{1}'", modelId, modelPath);
            return false;
        }

        // Validate compatibility
        if (!ValidateModelCompatibility(model)) {
            AGK_ERROR("AIManager: Model '{0}' failed compatibility validation", modelId);
            return false;
        }

        // Add to loaded models
        m_loadedModels[modelId] = model;
        m_currentVRAMUsage += model->GetMemoryUsageMB();

        // Register with faction
        RegisterModelWithFaction(modelId, factionId);

        AGK_INFO("AIManager: Successfully loaded model '{0}' for faction '{1}' (Memory: {2}MB, Total VRAM: {3}MB)",
            modelId, factionId, model->GetMemoryUsageMB(), m_currentVRAMUsage);

        return true;
    }

    void AIManager::RegisterModelWithFaction(const String& modelId, const String& factionId) {
        m_factionModels[factionId].push_back(modelId);
        AGK_TRACE("AIManager: Registered model '{0}' with faction '{1}'", modelId, factionId);
    }

    void AIManager::UnregisterModelFromFaction(const String& modelId, const String& factionId) {
        auto it = m_factionModels.find(factionId);
        if (it != m_factionModels.end()) {
            auto& modelIds = it->second;
            auto modelIt = std::find(modelIds.begin(), modelIds.end(), modelId);
            if (modelIt != modelIds.end()) {
                modelIds.erase(modelIt);
                AGK_TRACE("AIManager: Unregistered model '{0}' from faction '{1}'", modelId, factionId);
            }
        }
    }

} // namespace Angaraka::AI