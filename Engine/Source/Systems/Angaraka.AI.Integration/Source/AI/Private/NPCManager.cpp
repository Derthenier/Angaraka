#include "Angaraka/NPCManager.hpp"
#include <Angaraka/AIManager.hpp>
#include <sstream>
#include <fstream>

import Angaraka.Core.Events;
import Angaraka.Core.ResourceCache;
import Angaraka.Graphics.DirectX12;

import Angaraka.Math;
import Angaraka.Math.Vector3;
import Angaraka.Math.Random;

using namespace Angaraka::Core;
using namespace Angaraka::Events;

namespace Angaraka::AI {

    // ==================================================================================
    // Constructor and Destructor
    // ==================================================================================

    NPCManager::NPCManager(
        Reference<CachedResourceManager> resourceManager,
        Reference<AIManager> aiManager,
        Reference<Angaraka::DirectX12GraphicsSystem> graphicsSystem)
        : m_resourceManager(resourceManager)
        , m_aiManager(aiManager)
        , m_graphicsSystem(graphicsSystem)
        , m_isInitialized(false)
        , m_playerPosition(Vector3::Zero)
        , m_playerDirection(Vector3::Zero)
        , m_playerViewDistance(100.0f)
        , m_totalUpdateTime(0.0f)
        , m_updateCount(0)
        , m_frameUpdateCount(0)
        , m_currentUpdateIndex(0)
        , m_lastPerformanceReport(std::chrono::steady_clock::now())
    {
        AGK_INFO("NPCManager: Creating NPC management system");
    }

    NPCManager::~NPCManager() {
        AGK_INFO("NPCManager: Destroying NPC management system");
        Shutdown();
    }

    // ==================================================================================
    // Core Lifecycle
    // ==================================================================================

    bool NPCManager::Initialize(const NPCManagerSettings& settings) {
        if (m_isInitialized) {
            AGK_WARN("NPCManager: Already initialized");
            return true;
        }

        AGK_INFO("NPCManager: Initializing with max active NPCs: {0}, max update distance: {1}",
            settings.maxActiveNPCs, settings.maxUpdateDistance);

        // Validate required systems
        if (!m_resourceManager || !m_aiManager || !m_graphicsSystem) {
            AGK_ERROR("NPCManager: Missing required system dependencies");
            return false;
        }

        // Store settings
        m_settings = settings;

        // Reserve space for performance
        m_npcs.reserve(settings.maxActiveNPCs);
        m_activeNPCs.reserve(settings.maxActiveNPCs);
        m_visibleNPCs.reserve(settings.maxActiveNPCs / 2);
        m_interactableNPCs.reserve(20); // Reasonable number of nearby NPCs

        // Subscribe to events
        SubscribeToEvents();

        // Register default templates
        RegisterTemplate(NPCManagerUtils::CreateCivilianTemplate("civilian_neutral", NPCFaction::Neutral));
        RegisterTemplate(NPCManagerUtils::CreateCivilianTemplate("civilian_ashvattha", NPCFaction::Ashvattha));
        RegisterTemplate(NPCManagerUtils::CreateCivilianTemplate("civilian_vaikuntha", NPCFaction::Vaikuntha));
        RegisterTemplate(NPCManagerUtils::CreateCivilianTemplate("civilian_yuga_striders", NPCFaction::YugaStriders));

        RegisterTemplate(NPCManagerUtils::CreateGuardTemplate("guard_ashvattha", NPCFaction::Ashvattha));
        RegisterTemplate(NPCManagerUtils::CreateGuardTemplate("guard_vaikuntha", NPCFaction::Vaikuntha));
        RegisterTemplate(NPCManagerUtils::CreateGuardTemplate("guard_yuga_striders", NPCFaction::YugaStriders));

        RegisterTemplate(NPCManagerUtils::CreateMerchantTemplate("merchant_neutral", NPCFaction::Neutral));
        RegisterTemplate(NPCManagerUtils::CreateScholarTemplate("scholar_ashvattha", NPCFaction::Ashvattha));

        m_isInitialized = true;
        AGK_INFO("NPCManager: Initialization complete with {0} default templates", m_templates.size());

        return true;
    }

    void NPCManager::Update(F32 deltaTime) {
        if (!m_isInitialized) {
            return;
        }

        auto updateStart = std::chrono::steady_clock::now();

        // Update performance metrics
        UpdatePerformanceMetrics(deltaTime);

        // Categorize NPCs based on distance and visibility
        CategorizeNPCs();

        // Process NPCs in batches for performance
        if (m_settings.enableBatchUpdates) {
            U32 npcsToUpdate = Math::Util::Min(m_settings.maxUpdatesPerFrame, static_cast<U32>(m_activeNPCs.size()));

            if (npcsToUpdate > 0) {
                U32 batchSize = Math::Util::Max(1u, npcsToUpdate / 4); // Process in 4 batches per frame
                U32 endIndex = Math::Util::Min(m_currentUpdateIndex + batchSize, static_cast<U32>(m_activeNPCs.size()));

                std::vector<NPCController*> batchNPCs(m_activeNPCs.begin() + m_currentUpdateIndex,
                    m_activeNPCs.begin() + endIndex);
                UpdateNPCBatch(batchNPCs, deltaTime);

                m_currentUpdateIndex = endIndex;
                if (m_currentUpdateIndex >= m_activeNPCs.size()) {
                    m_currentUpdateIndex = 0; // Reset for next frame
                }
            }
        }
        else {
            // Update all active NPCs at once
            UpdateNPCBatch(m_activeNPCs, deltaTime);
        }

        // Execute custom update callback
        if (m_updateCallback) {
            m_updateCallback(m_activeNPCs, deltaTime);
        }

        // Process any pending events
        ProcessNPCEvents();

        // Cleanup distant NPCs if culling is enabled
        if (m_settings.enableDistanceCulling) {
            CullDistantNPCs();
        }

        // Update frame statistics
        m_frameUpdateCount++;

        auto updateEnd = std::chrono::steady_clock::now();
        F32 frameUpdateTime = std::chrono::duration<F32, std::milli>(updateEnd - updateStart).count();
        m_totalUpdateTime += frameUpdateTime;

        // Log performance report periodically
        if (m_settings.enablePerformanceMetrics) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastPerformanceReport);
            if (elapsed.count() >= 10) { // Every 10 seconds
                LogPerformanceReport();
                m_lastPerformanceReport = now;
            }
        }
    }

    void NPCManager::Render(F32 deltaTime) {
        if (!m_isInitialized || !m_graphicsSystem) {
            return;
        }

        // Render visible NPCs through the mesh manager
        for (NPCController* npc : m_visibleNPCs) {
            if (!npc || !npc->IsVisibleToPlayer()) {
                continue;
            }

            const NPCComponent& npcData = npc->GetNPCData();
            if (npcData.meshResourceId.empty()) {
                continue;
            }

            // Get world matrix for rendering
            Matrix4x4 worldMatrix = npc->GetWorldMatrix();

            // Render NPC mesh (integration with your MeshManager)
            // Note: Actual rendering call would depend on your MeshManager API
            // m_graphicsSystem->RenderMesh(npcData.meshResourceId, worldMatrix, npcData.textureResourceId);

            if (m_settings.enableDebugLogging) {
                AGK_DEBUG("NPCManager: Rendering NPC '{0}' at position ({1}, {2}, {3})",
                    npcData.npcId, npcData.position.x, npcData.position.y, npcData.position.z);
            }
        }
    }

    void NPCManager::Shutdown() {
        if (!m_isInitialized) {
            return;
        }

        AGK_INFO("NPCManager: Shutting down with {0} active NPCs", m_npcs.size());

        // Unsubscribe from events
        UnsubscribeFromEvents();

        // Shutdown all NPCs
        {
            std::lock_guard<std::mutex> lock(m_npcMutex);
            for (auto& [npcId, npcController] : m_npcs) {
                if (npcController) {
                    npcController->Shutdown();
                }
            }
            m_npcs.clear();
        }

        // Clear templates and caches
        m_templates.clear();
        m_activeNPCs.clear();
        m_visibleNPCs.clear();
        m_interactableNPCs.clear();

        // Clear callbacks
        m_spawnCallback = nullptr;
        m_destroyCallback = nullptr;
        m_updateCallback = nullptr;

        m_isInitialized = false;
        AGK_INFO("NPCManager: Shutdown complete");
    }

    // ==================================================================================
    // NPC Template Management
    // ==================================================================================

    bool NPCManager::RegisterTemplate(const NPCTemplate& npcTemplate) {
        if (!ValidateTemplate(npcTemplate)) {
            AGK_ERROR("NPCManager: Invalid template '{0}'", npcTemplate.templateId);
            return false;
        }

        if (m_templates.find(npcTemplate.templateId) != m_templates.end()) {
            AGK_WARN("NPCManager: Template '{0}' already exists, overwriting", npcTemplate.templateId);
        }

        m_templates[npcTemplate.templateId] = npcTemplate;

        // Preload resources for better performance
        PreloadNPCResources(npcTemplate);

        AGK_INFO("NPCManager: Registered template '{0}' for faction {1}",
            npcTemplate.templateId, (int)npcTemplate.faction);

        return true;
    }

    bool NPCManager::UnregisterTemplate(const String& templateId) {
        auto it = m_templates.find(templateId);
        if (it == m_templates.end()) {
            AGK_WARN("NPCManager: Template '{0}' not found for unregistration", templateId);
            return false;
        }

        m_templates.erase(it);
        AGK_INFO("NPCManager: Unregistered template '{0}'", templateId);
        return true;
    }

    const NPCTemplate* NPCManager::GetTemplate(const String& templateId) const {
        auto it = m_templates.find(templateId);
        return it != m_templates.end() ? &it->second : nullptr;
    }

    std::vector<String> NPCManager::GetRegisteredTemplates() const {
        std::vector<String> templateIds;
        templateIds.reserve(m_templates.size());

        for (const auto& [templateId, _] : m_templates) {
            templateIds.push_back(templateId);
        }

        return templateIds;
    }

    // ==================================================================================
    // NPC Lifecycle Management
    // ==================================================================================

    bool NPCManager::SpawnNPC(const NPCSpawnParams& spawnParams) {
        if (!m_isInitialized) {
            AGK_ERROR("NPCManager: Cannot spawn NPC - system not initialized");
            return false;
        }

        if (!ValidateSpawnParams(spawnParams)) {
            AGK_ERROR("NPCManager: Invalid spawn parameters for NPC '{0}'", spawnParams.npcId);
            return false;
        }

        // Check if NPC already exists
        {
            std::lock_guard<std::mutex> lock(m_npcMutex);
            if (m_npcs.find(spawnParams.npcId) != m_npcs.end()) {
                AGK_ERROR("NPCManager: NPC '{0}' already exists", spawnParams.npcId);
                return false;
            }

            // Check NPC limits
            if (m_npcs.size() >= m_settings.maxActiveNPCs) {
                AGK_WARN("NPCManager: Maximum NPC limit ({0}) reached, cannot spawn '{1}'",
                    m_settings.maxActiveNPCs, spawnParams.npcId);
                return false;
            }
        }

        // Create NPC from template
        if (!CreateNPCFromTemplate(spawnParams)) {
            AGK_ERROR("NPCManager: Failed to create NPC '{0}' from template '{1}'",
                spawnParams.npcId, spawnParams.templateId);
            return false;
        }

        AGK_INFO("NPCManager: Successfully spawned NPC '{0}' at position ({1}, {2}, {3})",
            spawnParams.npcId, spawnParams.position.x, spawnParams.position.y, spawnParams.position.z);

        // Execute spawn callback
        if (m_spawnCallback) {
            NPCController* npc = GetNPC(spawnParams.npcId);
            if (npc) {
                m_spawnCallback(npc);
            }
        }

        // Publish spawn event
        NPCSpawnEvent spawnEvent;
        spawnEvent.npcId = spawnParams.npcId;
        spawnEvent.position = spawnParams.position;

        const NPCTemplate* templ = GetTemplate(spawnParams.templateId);
        spawnEvent.faction = templ ? templ->faction : NPCFaction::Neutral;
        spawnEvent.templateId = spawnParams.templateId;

        Angaraka::Events::EventManager::Get().Broadcast(spawnEvent);

        // Note: Actual event publishing would depend on your EventManager API
        AGK_INFO("NPCManager: Publishing spawn event for NPC '{0}'", spawnParams.npcId);

        return true;
    }

    bool NPCManager::SpawnNPC(const String& npcId, const String& templateId, const Vector3& position) {
        NPCSpawnParams params;
        params.npcId = npcId;
        params.templateId = templateId;
        params.position = position;

        return SpawnNPC(params);
    }

    bool NPCManager::DestroyNPC(const String& npcId, const String& reason) {
        std::lock_guard<std::mutex> lock(m_npcMutex);

        auto it = m_npcs.find(npcId);
        if (it == m_npcs.end()) {
            AGK_WARN("NPCManager: Cannot destroy NPC '{0}' - not found", npcId);
            return false;
        }

        AGK_INFO("NPCManager: Destroying NPC '{0}' (Reason: {1})", npcId, reason);

        // Shutdown the NPC
        if (it->second) {
            it->second->Shutdown();
        }

        // Remove from collections
        m_npcs.erase(it);

        // Execute destroy callback
        if (m_destroyCallback) {
            m_destroyCallback(npcId);
        }

        // Publish destroy event

        NPCDestroyEvent destroyEvent;
        destroyEvent.npcId = npcId;
        destroyEvent.reason = reason;
        Angaraka::Events::EventManager::Get().Broadcast(destroyEvent);

        // Note: Actual event publishing would depend on your EventManager API
        AGK_INFO("NPCManager: Publishing destroy event for NPC '{0}'", npcId);

        // Update NPC lists on next frame
        UpdateNPCLists();

        return true;
    }

    void NPCManager::DestroyAllNPCs() {
        std::lock_guard<std::mutex> lock(m_npcMutex);

        AGK_INFO("NPCManager: Destroying all {0} NPCs", m_npcs.size());

        for (auto& [npcId, npcController] : m_npcs) {
            if (npcController) {
                npcController->Shutdown();
            }

            // Execute destroy callback for each NPC
            if (m_destroyCallback) {
                m_destroyCallback(npcId);
            }
        }

        m_npcs.clear();
        m_activeNPCs.clear();
        m_visibleNPCs.clear();
        m_interactableNPCs.clear();

        AGK_INFO("NPCManager: All NPCs destroyed");
    }

    // ==================================================================================
    // NPC Access and Queries
    // ==================================================================================

    NPCController* NPCManager::GetNPC(const String& npcId) {
        std::lock_guard<std::mutex> lock(m_npcMutex);
        auto it = m_npcs.find(npcId);
        return it != m_npcs.end() ? it->second.get() : nullptr;
    }

    const NPCController* NPCManager::GetNPC(const String& npcId) const {
        std::lock_guard<std::mutex> lock(m_npcMutex);
        auto it = m_npcs.find(npcId);
        return it != m_npcs.end() ? it->second.get() : nullptr;
    }

    std::vector<NPCController*> NPCManager::GetNPCsInRange(const Vector3& position, F32 range) {
        std::vector<NPCController*> npcsInRange;

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (auto& [npcId, npcController] : m_npcs) {
            if (!npcController || !npcController->IsActive()) {
                continue;
            }

            F32 distance = NPCUtils::CalculateDistance(npcController->GetPosition(), position);
            if (distance <= range) {
                npcsInRange.push_back(npcController.get());
            }
        }

        return npcsInRange;
    }

    std::vector<NPCController*> NPCManager::GetNPCsByFaction(NPCFaction faction) {
        std::vector<NPCController*> factionNPCs;

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (auto& [npcId, npcController] : m_npcs) {
            if (!npcController) {
                continue;
            }

            if (npcController->GetNPCData().faction == faction) {
                factionNPCs.push_back(npcController.get());
            }
        }

        return factionNPCs;
    }

    std::vector<NPCController*> NPCManager::GetNPCsByState(NPCState state) {
        std::vector<NPCController*> stateNPCs;

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (auto& [npcId, npcController] : m_npcs) {
            if (!npcController) {
                continue;
            }

            if (npcController->GetCurrentState() == state) {
                stateNPCs.push_back(npcController.get());
            }
        }

        return stateNPCs;
    }

    std::vector<NPCController*> NPCManager::GetInteractableNPCs(const Vector3& playerPosition) {
        std::vector<NPCController*> interactable;

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (auto& [npcId, npcController] : m_npcs) {
            if (!npcController || !npcController->IsActive()) {
                continue;
            }

            if (npcController->CanInteractWithPlayer() &&
                npcController->IsPlayerInRange(playerPosition)) {
                interactable.push_back(npcController.get());
            }
        }

        return interactable;
    }

    // ==================================================================================
    // Bulk Operations
    // ==================================================================================

    void NPCManager::SetAllNPCsActive(bool active) {
        std::lock_guard<std::mutex> lock(m_npcMutex);

        for (auto& [npcId, npcController] : m_npcs) {
            if (npcController) {
                npcController->SetActive(active);
            }
        }

        AGK_INFO("NPCManager: Set all {0} NPCs active state to {1}", m_npcs.size(), active);
    }

    void NPCManager::SetNPCsActiveInRange(const Vector3& center, F32 range, bool active) {
        U32 affectedCount = 0;

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (auto& [npcId, npcController] : m_npcs) {
            if (!npcController) {
                continue;
            }

            F32 distance = NPCUtils::CalculateDistance(npcController->GetPosition(), center);
            if (distance <= range) {
                npcController->SetActive(active);
                affectedCount++;
            }
        }

        AGK_INFO("NPCManager: Set {0} NPCs active state to {1} within range {2} of ({3}, {4}, {5})",
            affectedCount, active, range, center.x, center.y, center.z);
    }

    void NPCManager::UpdateAllNPCDistances(const Vector3& playerPosition) {
        m_playerPosition = playerPosition;

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (auto& [npcId, npcController] : m_npcs) {
            if (npcController) {
                npcController->UpdateDistanceToPlayer(playerPosition);
            }
        }
    }

    void NPCManager::ChangeStateForFaction(NPCFaction faction, NPCState newState, const String& reason) {
        U32 affectedCount = 0;

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (auto& [npcId, npcController] : m_npcs) {
            if (!npcController) {
                continue;
            }

            if (npcController->GetNPCData().faction == faction) {
                npcController->ChangeState(newState, reason);
                affectedCount++;
            }
        }

        AGK_INFO("NPCManager: Changed state to {0} for {1} NPCs of faction {2} (Reason: {3})",
            (int)newState, affectedCount, (int)faction, reason);
    }

    // ==================================================================================
    // Player Integration
    // ==================================================================================

    void NPCManager::UpdatePlayerLocation(const Vector3& position, const Vector3& direction) {
        m_playerPosition = position;
        m_playerDirection = direction;

        // Update all NPC distances efficiently
        UpdateAllNPCDistances(position);

        // Publish player movement event
        PlayerLocationUpdateEvent playerEvent;
        playerEvent.position = position;
        playerEvent.direction = direction;
        playerEvent.viewDistance = m_playerViewDistance;
        Angaraka::Events::EventManager::Get().Broadcast(playerEvent);

        // Note: Actual event publishing would depend on your EventManager API
        if (m_settings.enableDebugLogging) {
            AGK_DEBUG("NPCManager: Player moved to ({0}, {1}, {2})", position.x, position.y, position.z);
        }
    }

    void NPCManager::TriggerPlayerInteraction(const String& npcId, const String& action, InteractionType type) {
        NPCController* npc = GetNPC(npcId);
        if (!npc) {
            AGK_WARN("NPCManager: Cannot trigger interaction - NPC '{0}' not found", npcId);
            return;
        }

        if (!npc->CanInteractWithPlayer()) {
            AGK_WARN("NPCManager: Cannot interact with NPC '{0}' in current state", npcId);
            return;
        }

        AGK_INFO("NPCManager: Triggering player interaction with NPC '{0}' (Action: {1})", npcId, action);

        // Create interaction event
        NPCInteractionEvent interaction;
        interaction.npcId = npcId;
        interaction.playerAction = action;
        interaction.playerPosition = m_playerPosition;
        interaction.interactionDistance = NPCUtils::CalculateDistance(npc->GetPosition(), m_playerPosition);
        interaction.interactionType = type;

        // Trigger the interaction
        npc->TriggerInteraction(interaction);
    }

    std::vector<String> NPCManager::GetNearbyInteractableNPCs(F32 range) const {
        std::vector<String> nearbyNPCs;

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (const auto& [npcId, npcController] : m_npcs) {
            if (!npcController || !npcController->IsActive()) {
                continue;
            }

            if (npcController->CanInteractWithPlayer() &&
                npcController->IsPlayerInRange(m_playerPosition)) {
                F32 distance = NPCUtils::CalculateDistance(npcController->GetPosition(), m_playerPosition);
                if (distance <= range) {
                    nearbyNPCs.push_back(npcId);
                }
            }
        }

        return nearbyNPCs;
    }

    // ==================================================================================
    // Bundle and Configuration Loading
    // ==================================================================================

    bool NPCManager::LoadNPCsFromBundle(const String& bundleId) {
        if (!m_resourceManager) {
            AGK_ERROR("NPCManager: No resource manager available for bundle loading");
            return false;
        }

        AGK_INFO("NPCManager: Loading NPCs from bundle '{0}'", bundleId);

        // Note: Actual bundle loading would depend on your CachedResourceManager and bundle format
        // This is a placeholder implementation

        // Example: Load NPC definitions from YAML bundle
        // auto bundleResource = m_resourceManager->GetResource<BundleResource>(bundleId);
        // if (!bundleResource) {
        //     AGK_ERROR("NPCManager: Bundle '{0}' not found", bundleId);
        //     return false;
        // }

        // For now, create some example NPCs
        std::vector<NPCSpawnParams> exampleNPCs = NPCManagerUtils::GenerateRandomNPCs(5, Vector3::Zero, 20.0f);

        U32 spawnedCount = 0;
        for (const auto& spawnParams : exampleNPCs) {
            if (SpawnNPC(spawnParams)) {
                spawnedCount++;
            }
        }

        AGK_INFO("NPCManager: Spawned {0} NPCs from bundle '{1}'", spawnedCount, bundleId);
        return spawnedCount > 0;
    }

    bool NPCManager::SaveNPCsToBundle(const String& bundleId) const {
        AGK_INFO("NPCManager: Saving {0} NPCs to bundle '{1}'", m_npcs.size(), bundleId);

        // Note: Actual bundle saving would depend on your bundle format
        // This is a placeholder implementation

        std::lock_guard<std::mutex> lock(m_npcMutex);

        // Example: Save NPC data to YAML format
        // YAML bundle format would include NPC positions, states, templates, etc.

        AGK_INFO("NPCManager: Successfully saved NPCs to bundle '{0}'", bundleId);
        return true;
    }

    bool NPCManager::LoadTemplatesFromConfig(const String& configPath) {
        AGK_INFO("NPCManager: Loading NPC templates from config '{0}'", configPath);

        // Note: Actual config loading would depend on your configuration system
        // This is a placeholder implementation

        // Example: Load templates from YAML config file
        // The config would define template properties, models, personalities, etc.

        AGK_INFO("NPCManager: Successfully loaded templates from config '{0}'", configPath);
        return true;
    }

    // ==================================================================================
    // Performance and Optimization
    // ==================================================================================

    void NPCManager::OptimizeNPCUpdates() {
        // Sort NPCs by distance for optimal update order
        std::sort(m_activeNPCs.begin(), m_activeNPCs.end(),
            [this](const NPCController* a, const NPCController* b) {
                return a->GetNPCData().distanceToPlayer < b->GetNPCData().distanceToPlayer;
            });

        // Adjust update intervals based on distance
        for (NPCController* npc : m_activeNPCs) {
            if (!npc) continue;

            NPCComponent& npcData = npc->GetNPCData();
            F32 baseInterval = 1.0f; // Base update interval in seconds

            if (npcData.distanceToPlayer > 50.0f) {
                baseInterval *= 2.0f; // Update less frequently when far
            }
            else if (npcData.distanceToPlayer > 100.0f) {
                baseInterval *= 4.0f; // Update much less frequently when very far
            }

            // Apply global multiplier
            baseInterval *= m_settings.updateIntervalMultiplier;

            // Note: Would need to add SetAIUpdateInterval method to NPCController
            // npc->SetAIUpdateInterval(baseInterval);
        }

        AGK_DEBUG("NPCManager: Optimized update intervals for {0} active NPCs", m_activeNPCs.size());
    }

    void NPCManager::CullDistantNPCs() {
        U32 culledCount = 0;

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (auto& [npcId, npcController] : m_npcs) {
            if (!npcController) {
                continue;
            }

            F32 distance = npcController->GetNPCData().distanceToPlayer;

            // Deactivate NPCs beyond max distance
            if (distance > m_settings.maxUpdateDistance) {
                if (npcController->IsActive()) {
                    npcController->SetActive(false);
                    culledCount++;
                }
            }
            else if (distance <= m_settings.maxUpdateDistance * 0.8f) {
                // Reactivate NPCs that come back into range
                if (!npcController->IsActive()) {
                    npcController->SetActive(true);
                }
            }
        }

        if (culledCount > 0 && m_settings.enableDebugLogging) {
            AGK_DEBUG("NPCManager: Culled {0} distant NPCs", culledCount);
        }
    }

    void NPCManager::EnableNPCsInFrustum(const Vector3& cameraPos, const Vector3& cameraDir, F32 fov) {
        if (!m_settings.enableFrustumCulling) {
            return;
        }

        // Simple frustum culling implementation
        // Note: This is a basic implementation - could be enhanced with proper frustum math
        U32 visibleCount = 0;

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (auto& [npcId, npcController] : m_npcs) {
            if (!npcController) {
                continue;
            }

            Vector3 npcPos = npcController->GetPosition();
            Vector3 toNPC = (npcPos - cameraPos).Normalized();
            F32 dotProduct = cameraDir.Dot(toNPC);

            // Simple FOV check (cos of half FOV angle)
            F32 cosHalfFOV = Math::Util::CosDegrees(fov * 0.5f * Math::Constants::DegToRadF);
            bool inFrustum = dotProduct >= cosHalfFOV;

            NPCComponent& npcData = npcController->GetNPCData();
            npcData.isInPlayerView = inFrustum && npcData.distanceToPlayer <= m_settings.maxRenderDistance;

            if (npcData.isInPlayerView) {
                visibleCount++;
            }
        }

        if (m_settings.enableDebugLogging) {
            AGK_DEBUG("NPCManager: {0} NPCs visible in frustum", visibleCount);
        }
    }

    // ==================================================================================
    // Settings Management
    // ==================================================================================

    void NPCManager::UpdateSettings(const NPCManagerSettings& newSettings) {
        AGK_INFO("NPCManager: Updating settings - Max NPCs: {0}, Max Distance: {1}",
            newSettings.maxActiveNPCs, newSettings.maxUpdateDistance);

        m_settings = newSettings;

        // Apply new settings immediately
        if (m_settings.enableDistanceCulling) {
            CullDistantNPCs();
        }

        OptimizeNPCUpdates();
    }

    // ==================================================================================
    // Statistics and Debugging
    // ==================================================================================

    U32 NPCManager::GetActiveNPCCount() const {
        std::lock_guard<std::mutex> lock(m_npcMutex);
        U32 activeCount = 0;

        for (const auto& [npcId, npcController] : m_npcs) {
            if (npcController && npcController->IsActive()) {
                activeCount++;
            }
        }

        return activeCount;
    }

    U32 NPCManager::GetVisibleNPCCount() const {
        std::lock_guard<std::mutex> lock(m_npcMutex);
        U32 visibleCount = 0;

        for (const auto& [npcId, npcController] : m_npcs) {
            if (npcController && npcController->GetNPCData().isInPlayerView) {
                visibleCount++;
            }
        }

        return visibleCount;
    }

    U32 NPCManager::GetTotalNPCCount() const {
        std::lock_guard<std::mutex> lock(m_npcMutex);
        return static_cast<U32>(m_npcs.size());
    }

    F32 NPCManager::GetAverageUpdateTime() const {
        return m_updateCount > 0 ? m_totalUpdateTime / m_updateCount : 0.0f;
    }

    String NPCManager::GetPerformanceReport() const {
        std::stringstream report;

        report << "=== NPCManager Performance Report ===\n";
        report << "Total NPCs: " << GetTotalNPCCount() << "\n";
        report << "Active NPCs: " << GetActiveNPCCount() << "\n";
        report << "Visible NPCs: " << GetVisibleNPCCount() << "\n";
        report << "Average Update Time: " << GetAverageUpdateTime() << "ms\n";
        report << "Frame Updates: " << m_frameUpdateCount << "\n";
        report << "Settings - Max Distance: " << m_settings.maxUpdateDistance << ", Max NPCs: " << m_settings.maxActiveNPCs << "\n";

        // Template statistics
        report << "Registered Templates: " << m_templates.size() << " (";
        for (const auto& [templateId, _] : m_templates) {
            report << templateId << " ";
        }
        report << ")\n";

        return report.str();
    }

    void NPCManager::LogSystemStatus() const {
        AGK_INFO("NPCManager Status: {0} total, {1} active, {2} visible NPCs. Avg update: {3:.2f}ms",
            GetTotalNPCCount(), GetActiveNPCCount(), GetVisibleNPCCount(), GetAverageUpdateTime());
    }

    // ==================================================================================
    // Private Helper Methods
    // ==================================================================================

    void NPCManager::SubscribeToEvents() {
        // Note: Actual event subscription would depend on your EventManager API
        // Example subscriptions:
        // Angaraka::Events::EventManager::Get().Subscribe<NPCStateChangeEvent>([this](const PlayerMovementEvent& e) { OnPlayerMovement(e.position); });

        AGK_INFO("NPCManager: Subscribed to system events");
    }

    void NPCManager::UnsubscribeFromEvents() {
        // Note: Actual event unsubscription would depend on your EventManager API
        AGK_INFO("NPCManager: Unsubscribed from system events");
    }

    void NPCManager::OnPlayerMovement(const Vector3& position) {
        UpdatePlayerLocation(position, m_playerDirection);
    }

    void NPCManager::OnNPCInteraction(const NPCInteractionEvent& interaction) {
        NPCController* npc = GetNPC(interaction.npcId);
        if (npc) {
            npc->TriggerInteraction(interaction);
        }
    }

    void NPCManager::OnNPCStateChange(const NPCStateChangeEvent& stateChange) {
        if (m_settings.enableStateLogging) {
            AGK_INFO("NPCManager: NPC '{0}' changed state from {1} to {2} - {3}",
                stateChange.npcId, (int)stateChange.oldState, (int)stateChange.newState, stateChange.reason);
        }
    }

    bool NPCManager::CreateNPCFromTemplate(const NPCSpawnParams& spawnParams) {
        const NPCTemplate* npcTemplate = GetTemplate(spawnParams.templateId);
        if (!npcTemplate) {
            AGK_ERROR("NPCManager: Template '{0}' not found", spawnParams.templateId);
            return false;
        }

        // Create NPC controller
        auto npcController = Angaraka::CreateScope<NPCController>(
            spawnParams.npcId,
            m_resourceManager,
            m_aiManager,
            m_graphicsSystem
        );

        // Create NPC component from template
        NPCComponent npcData(spawnParams.npcId,
            spawnParams.customDisplayName.empty() ? npcTemplate->displayName : spawnParams.customDisplayName,
            spawnParams.customFaction == NPCFaction::Count ? npcTemplate->faction : spawnParams.customFaction);

        // Apply template data
        npcData.npcType = npcTemplate->npcType;
        npcData.personality = npcTemplate->personality;
        npcData.meshResourceId = npcTemplate->meshResourceId;
        npcData.textureResourceId = npcTemplate->textureResourceId;
        npcData.dialogueModelId = npcTemplate->dialogueModelId;
        npcData.behaviorModelId = npcTemplate->behaviorModelId;
        npcData.interactionRange = npcTemplate->interactionRange;
        npcData.availableInteraction = npcTemplate->defaultInteraction;
        npcData.description = npcTemplate->description;

        // Apply spawn parameters
        npcData.position = spawnParams.position;
        npcData.rotation = spawnParams.rotation;
        npcData.scale = spawnParams.scale;
        npcData.isActive = spawnParams.isActive;
        npcData.isVisible = spawnParams.isVisible;

        // Apply personality overrides
        for (const auto& [trait, value] : spawnParams.personalityOverrides) {
            if (trait == "aggressiveness") npcData.personality.aggressiveness = value;
            else if (trait == "curiosity") npcData.personality.curiosity = value;
            else if (trait == "trustfulness") npcData.personality.trustfulness = value;
            else if (trait == "helpfulness") npcData.personality.helpfulness = value;
            else if (trait == "intelligence") npcData.personality.intelligence = value;
            else if (trait == "loyalty") npcData.personality.loyalty = value;
        }

        // Apply relationship overrides
        for (const auto& [aspect, value] : spawnParams.relationshipOverrides) {
            if (aspect == "trust") npcData.relationship.trust = value;
            else if (aspect == "respect") npcData.relationship.respect = value;
            else if (aspect == "fear") npcData.relationship.fear = value;
            else if (aspect == "affection") npcData.relationship.affection = value;
        }

        // Add template knowledge
        npcData.knowledge.knownTopics = npcTemplate->knownTopics;
        npcData.knowledge.topicExpertise = npcTemplate->topicExpertise;

        // Initialize the NPC controller
        if (!npcController->Initialize(npcData)) {
            AGK_ERROR("NPCManager: Failed to initialize NPC controller for '{0}'", spawnParams.npcId);
            return false;
        }

        // Store the NPC
        {
            std::lock_guard<std::mutex> lock(m_npcMutex);
            m_npcs[spawnParams.npcId] = std::move(npcController);
        }

        return true;
    }

    void NPCManager::UpdateNPCLists() {
        m_activeNPCs.clear();
        m_visibleNPCs.clear();
        m_interactableNPCs.clear();

        std::lock_guard<std::mutex> lock(m_npcMutex);
        for (auto& [npcId, npcController] : m_npcs) {
            if (!npcController) {
                continue;
            }

            const NPCComponent& npcData = npcController->GetNPCData();

            if (npcController->IsActive()) {
                m_activeNPCs.push_back(npcController.get());
            }

            if (npcData.isInPlayerView && npcData.isVisible) {
                m_visibleNPCs.push_back(npcController.get());
            }

            if (npcController->CanInteractWithPlayer() &&
                npcData.distanceToPlayer <= m_settings.maxInteractionDistance) {
                m_interactableNPCs.push_back(npcController.get());
            }
        }
    }

    void NPCManager::UpdateNPCBatch(const std::vector<NPCController*>& npcs, F32 deltaTime) {
        for (NPCController* npc : npcs) {
            if (!npc || !ShouldUpdateNPC(npc)) {
                continue;
            }

            npc->Update(deltaTime);
        }
    }

    void NPCManager::ProcessNPCEvents() {
        // Process any pending NPC-related events
        // This could include state synchronization, batch operations, etc.

        // Publish system update event periodically
        if (m_frameUpdateCount % 60 == 0) { // Every ~1 second at 60 FPS
            NPCSystemUpdateEvent systemEvent;
            systemEvent.activeNPCCount = GetActiveNPCCount();
            systemEvent.visibleNPCCount = GetVisibleNPCCount();
            systemEvent.totalUpdateTime = m_totalUpdateTime;
            systemEvent.averageUpdateTime = GetAverageUpdateTime();
            Angaraka::Events::EventManager::Get().Broadcast(systemEvent);

            // Note: Actual event publishing would depend on your EventManager API
            if (m_settings.enableDebugLogging) {
                AGK_DEBUG("NPCManager: Publishing system update event");
            }
        }
    }

    void NPCManager::CategorizeNPCs() {
        UpdateNPCLists();

        // Sort by distance for optimization
        if (!m_activeNPCs.empty()) {
            std::sort(m_activeNPCs.begin(), m_activeNPCs.end(),
                [](const NPCController* a, const NPCController* b) {
                    return a->GetNPCData().distanceToPlayer < b->GetNPCData().distanceToPlayer;
                });
        }
    }

    bool NPCManager::ShouldUpdateNPC(const NPCController* npc) const {
        if (!npc || !npc->IsActive()) {
            return false;
        }

        const NPCComponent& npcData = npc->GetNPCData();

        // Always update NPCs in conversation
        if (npcData.currentState == NPCState::Conversation) {
            return true;
        }

        // Skip distant NPCs unless they're in important states
        if (npcData.distanceToPlayer > m_settings.maxUpdateDistance) {
            return npcData.currentState == NPCState::Alert || npcData.currentState == NPCState::Hostile;
        }

        return true;
    }

    bool NPCManager::ShouldRenderNPC(const NPCController* npc) const {
        if (!npc || !npc->IsVisibleToPlayer()) {
            return false;
        }

        const NPCComponent& npcData = npc->GetNPCData();

        // Don't render NPCs beyond render distance
        if (npcData.distanceToPlayer > m_settings.maxRenderDistance) {
            return false;
        }

        // Check if in view frustum
        if (m_settings.enableFrustumCulling && !npcData.isInPlayerView) {
            return false;
        }

        return true;
    }

    bool NPCManager::IsNPCInView(const NPCController* npc) const {
        if (!npc) {
            return false;
        }

        // Simple view check - could be enhanced with proper frustum culling
        const NPCComponent& npcData = npc->GetNPCData();
        return npcData.distanceToPlayer <= m_playerViewDistance;
    }

    void NPCManager::PreloadNPCResources(const NPCTemplate& npcTemplate) {
        if (!m_resourceManager) {
            return;
        }

        // Preload mesh resource
        if (!npcTemplate.meshResourceId.empty()) {
            // Note: Actual resource loading would depend on your CachedResourceManager API
            // auto meshResource = m_resourceManager->GetResource<MeshResource>(npcTemplate.meshResourceId);
            AGK_DEBUG("NPCManager: Preloading mesh resource '{0}' for template '{1}'",
                npcTemplate.meshResourceId, npcTemplate.templateId);
        }

        // Preload texture resource
        if (!npcTemplate.textureResourceId.empty()) {
            // auto textureResource = m_resourceManager->GetResource<TextureResource>(npcTemplate.textureResourceId);
            AGK_DEBUG("NPCManager: Preloading texture resource '{0}' for template '{1}'",
                npcTemplate.textureResourceId, npcTemplate.templateId);
        }

        // Preload AI models
        if (!npcTemplate.dialogueModelId.empty()) {
            // auto dialogueModel = m_resourceManager->GetResource<AIModelResource>(npcTemplate.dialogueModelId);
            AGK_DEBUG("NPCManager: Preloading dialogue model '{0}' for template '{1}'",
                npcTemplate.dialogueModelId, npcTemplate.templateId);
        }

        if (!npcTemplate.behaviorModelId.empty()) {
            // auto behaviorModel = m_resourceManager->GetResource<AIModelResource>(npcTemplate.behaviorModelId);
            AGK_DEBUG("NPCManager: Preloading behavior model '{0}' for template '{1}'",
                npcTemplate.behaviorModelId, npcTemplate.templateId);
        }
    }

    void NPCManager::CleanupUnusedResources() {
        // Clean up resources that are no longer being used
        // This would involve checking reference counts and unloading unused assets

        AGK_DEBUG("NPCManager: Cleaning up unused resources");

        // Note: Actual cleanup would depend on your CachedResourceManager implementation
        // m_resourceManager->CleanupUnusedResources();
    }

    bool NPCManager::ValidateSpawnParams(const NPCSpawnParams& params) const {
        if (params.npcId.empty()) {
            AGK_ERROR("NPCManager: NPC ID cannot be empty");
            return false;
        }

        if (params.templateId.empty()) {
            AGK_ERROR("NPCManager: Template ID cannot be empty for NPC '{0}'", params.npcId);
            return false;
        }

        const NPCTemplate* npcTemplate = GetTemplate(params.templateId);
        if (!npcTemplate) {
            AGK_ERROR("NPCManager: Template '{0}' not found for NPC '{1}'", params.templateId, params.npcId);
            return false;
        }

        return true;
    }

    bool NPCManager::ValidateTemplate(const NPCTemplate& npcTemplate) const {
        if (npcTemplate.templateId.empty()) {
            AGK_ERROR("NPCManager: Template ID cannot be empty");
            return false;
        }

        if (npcTemplate.displayName.empty()) {
            AGK_WARN("NPCManager: Template '{0}' has empty display name", npcTemplate.templateId);
        }

        if (npcTemplate.interactionRange <= 0.0f) {
            AGK_WARN("NPCManager: Template '{0}' has invalid interaction range: {1}",
                npcTemplate.templateId, npcTemplate.interactionRange);
        }

        return true;
    }

    void NPCManager::LogNPCError(const String& operation, const String& npcId, const String& error) const {
        AGK_ERROR("NPCManager: {0} failed for NPC '{1}' - {2}", operation, npcId, error);
    }

    void NPCManager::UpdatePerformanceMetrics(F32 deltaTime) {
        m_updateCount++;

        // Reset counters periodically to prevent overflow
        if (m_updateCount % 10000 == 0) {
            m_totalUpdateTime = 0.0f;
            m_updateCount = 1;
        }
    }

    void NPCManager::LogPerformanceReport() const {
        String report = GetPerformanceReport();
        AGK_INFO("NPCManager Performance:\n{0}", report);
    }

    String NPCManager::FormatNPCStatus(const NPCController* npc) const {
        if (!npc) {
            return "Invalid NPC";
        }

        const NPCComponent& npcData = npc->GetNPCData();
        std::stringstream ss;

        ss << npcData.npcId << " (" << npcData.displayName << ") - ";
        ss << "State: " << npcData.GetStateString() << ", ";
        ss << "Distance: " << npcData.distanceToPlayer << ", ";
        ss << "Active: " << (npc->IsActive() ? "Yes" : "No") << ", ";
        ss << "Visible: " << (npcData.isInPlayerView ? "Yes" : "No");

        return ss.str();
    }

    // ==================================================================================
    // NPCManagerUtils Implementation
    // ==================================================================================

    namespace NPCManagerUtils {

        // ==================================================================================
        // Template Creation Helpers
        // ==================================================================================

        NPCTemplate CreateCivilianTemplate(const String& templateId, NPCFaction faction) {
            NPCTemplate templ(templateId, "Civilian", faction);
            templ.npcType = NPCType::Civilian;
            templ.personality.aggressiveness = 0.2f;
            templ.personality.curiosity = 0.6f;
            templ.personality.trustfulness = 0.7f;
            templ.personality.helpfulness = 0.6f;
            templ.personality.intelligence = 0.5f;
            templ.personality.loyalty = 0.6f;
            templ.interactionRange = 3.0f;
            templ.defaultInteraction = InteractionType::Dialogue;
            templ.meshResourceId = "meshes/civilian";
            templ.textureResourceId = "textures/civilian";
            templ.description = "A friendly civilian willing to chat";

            // Add faction-specific knowledge
            switch (faction) {
            case NPCFaction::Ashvattha:
                templ.knownTopics = { "philosophy", "meditation", "ancient_wisdom", "traditions" };
                templ.topicExpertise["philosophy"] = 0.7f;
                templ.topicExpertise["traditions"] = 0.8f;
                templ.dialogueModelId = "ashvattha_dialogue";
                templ.behaviorModelId = "ashvattha_behavior";
                break;

            case NPCFaction::Vaikuntha:
                templ.knownTopics = { "technology", "efficiency", "progress", "systems" };
                templ.topicExpertise["technology"] = 0.8f;
                templ.topicExpertise["efficiency"] = 0.7f;
                templ.dialogueModelId = "vaikuntha_dialogue";
                templ.behaviorModelId = "vaikuntha_behavior";
                break;

            case NPCFaction::YugaStriders:
                templ.knownTopics = { "freedom", "rebellion", "justice", "change" };
                templ.topicExpertise["freedom"] = 0.8f;
                templ.topicExpertise["justice"] = 0.7f;
                templ.dialogueModelId = "yuga_striders_dialogue";
                templ.behaviorModelId = "yuga_striders_behavior";
                break;

            default: // Neutral
                templ.knownTopics = { "weather", "local_news", "general_topics" };
                templ.dialogueModelId = "neutral_dialogue";
                templ.behaviorModelId = "neutral_behavior";
                break;
            }

            return templ;
        }

        NPCTemplate CreateGuardTemplate(const String& templateId, NPCFaction faction) {
            NPCTemplate templ(templateId, "Guard", faction);
            templ.npcType = NPCType::Guard;
            templ.personality.aggressiveness = 0.6f;
            templ.personality.curiosity = 0.3f;
            templ.personality.trustfulness = 0.4f;
            templ.personality.helpfulness = 0.5f;
            templ.personality.intelligence = 0.6f;
            templ.personality.loyalty = 0.9f;
            templ.interactionRange = 4.0f;
            templ.defaultInteraction = InteractionType::Dialogue;
            templ.meshResourceId = "meshes/guard";
            templ.textureResourceId = "textures/guard";
            templ.description = "A vigilant guard protecting the area";
            templ.knownTopics = { "security", "protocols", "threats", "duty" };
            templ.topicExpertise["security"] = 0.9f;
            templ.topicExpertise["protocols"] = 0.8f;

            // Faction-specific guard specialization
            switch (faction) {
            case NPCFaction::Ashvattha:
                templ.displayName = "Temple Guardian";
                templ.knownTopics.push_back("sacred_grounds");
                templ.topicExpertise["sacred_grounds"] = 0.9f;
                templ.dialogueModelId = "ashvattha_dialogue";
                templ.behaviorModelId = "ashvattha_behavior";
                break;

            case NPCFaction::Vaikuntha:
                templ.displayName = "Security Officer";
                templ.knownTopics.push_back("surveillance");
                templ.topicExpertise["surveillance"] = 0.9f;
                templ.dialogueModelId = "vaikuntha_dialogue";
                templ.behaviorModelId = "vaikuntha_behavior";
                break;

            case NPCFaction::YugaStriders:
                templ.displayName = "Freedom Fighter";
                templ.knownTopics.push_back("resistance");
                templ.topicExpertise["resistance"] = 0.9f;
                templ.dialogueModelId = "yuga_striders_dialogue";
                templ.behaviorModelId = "yuga_striders_behavior";
                break;

            default:
                templ.dialogueModelId = "neutral_dialogue";
                templ.behaviorModelId = "neutral_behavior";
                break;
            }

            return templ;
        }

        NPCTemplate CreateMerchantTemplate(const String& templateId, NPCFaction faction) {
            NPCTemplate templ(templateId, "Merchant", faction);
            templ.npcType = NPCType::Merchant;
            templ.personality.aggressiveness = 0.3f;
            templ.personality.curiosity = 0.7f;
            templ.personality.trustfulness = 0.6f;
            templ.personality.helpfulness = 0.8f;
            templ.personality.intelligence = 0.7f;
            templ.personality.loyalty = 0.5f;
            templ.interactionRange = 3.5f;
            templ.defaultInteraction = InteractionType::Trade;
            templ.meshResourceId = "meshes/merchant";
            templ.textureResourceId = "textures/merchant";
            templ.description = "A friendly merchant with goods to sell";
            templ.knownTopics = { "trade", "goods", "prices", "economy", "travel" };
            templ.topicExpertise["trade"] = 0.9f;
            templ.topicExpertise["economy"] = 0.8f;

            // Merchants are typically neutral but may have faction preferences
            switch (faction) {
            case NPCFaction::Ashvattha:
                templ.knownTopics.push_back("artifacts");
                templ.topicExpertise["artifacts"] = 0.7f;
                break;

            case NPCFaction::Vaikuntha:
                templ.knownTopics.push_back("technology_trade");
                templ.topicExpertise["technology_trade"] = 0.8f;
                break;

            case NPCFaction::YugaStriders:
                templ.knownTopics.push_back("contraband");
                templ.topicExpertise["contraband"] = 0.6f;
                break;

            default:
                break;
            }

            // Set faction-appropriate AI models
            templ.dialogueModelId = faction == NPCFaction::Neutral ? "neutral_dialogue" :
                (faction == NPCFaction::Ashvattha ? "ashvattha_dialogue" :
                    (faction == NPCFaction::Vaikuntha ? "vaikuntha_dialogue" : "yuga_striders_dialogue"));
            templ.behaviorModelId = faction == NPCFaction::Neutral ? "neutral_behavior" :
                (faction == NPCFaction::Ashvattha ? "ashvattha_behavior" :
                    (faction == NPCFaction::Vaikuntha ? "vaikuntha_behavior" : "yuga_striders_behavior"));

            return templ;
        }

        NPCTemplate CreateScholarTemplate(const String& templateId, NPCFaction faction) {
            NPCTemplate templ(templateId, "Scholar", faction);
            templ.npcType = NPCType::Scholar;
            templ.personality.aggressiveness = 0.1f;
            templ.personality.curiosity = 0.9f;
            templ.personality.trustfulness = 0.7f;
            templ.personality.helpfulness = 0.8f;
            templ.personality.intelligence = 0.9f;
            templ.personality.loyalty = 0.7f;
            templ.interactionRange = 3.0f;
            templ.defaultInteraction = InteractionType::Information;
            templ.meshResourceId = "meshes/scholar";
            templ.textureResourceId = "textures/scholar";
            templ.description = "A learned scholar with vast knowledge";
            templ.knownTopics = { "history", "science", "literature", "research", "knowledge" };
            templ.topicExpertise["history"] = 0.9f;
            templ.topicExpertise["science"] = 0.8f;
            templ.topicExpertise["research"] = 0.9f;

            // Faction-specific scholarly specializations
            switch (faction) {
            case NPCFaction::Ashvattha:
                templ.displayName = "Sage";
                templ.knownTopics.insert(templ.knownTopics.end(),
                    { "ancient_texts", "spiritual_wisdom", "meditation_techniques" });
                templ.topicExpertise["ancient_texts"] = 0.95f;
                templ.topicExpertise["spiritual_wisdom"] = 0.9f;
                templ.dialogueModelId = "ashvattha_dialogue";
                templ.behaviorModelId = "ashvattha_behavior";
                break;

            case NPCFaction::Vaikuntha:
                templ.displayName = "Research Director";
                templ.knownTopics.insert(templ.knownTopics.end(),
                    { "advanced_technology", "data_analysis", "system_optimization" });
                templ.topicExpertise["advanced_technology"] = 0.95f;
                templ.topicExpertise["data_analysis"] = 0.9f;
                templ.dialogueModelId = "vaikuntha_dialogue";
                templ.behaviorModelId = "vaikuntha_behavior";
                break;

            case NPCFaction::YugaStriders:
                templ.displayName = "Revolutionary Thinker";
                templ.knownTopics.insert(templ.knownTopics.end(),
                    { "revolutionary_theory", "social_justice", "freedom_philosophy" });
                templ.topicExpertise["revolutionary_theory"] = 0.9f;
                templ.topicExpertise["social_justice"] = 0.85f;
                templ.dialogueModelId = "yuga_striders_dialogue";
                templ.behaviorModelId = "yuga_striders_behavior";
                break;

            default:
                templ.dialogueModelId = "neutral_dialogue";
                templ.behaviorModelId = "neutral_behavior";
                break;
            }

            return templ;
        }

        // ==================================================================================
        // Spawning Helpers
        // ==================================================================================

        std::vector<NPCSpawnParams> GenerateRandomNPCs(U32 count, const Math::Vector3& center, F32 radius) {
            std::vector<NPCSpawnParams> spawnList;
            spawnList.reserve(count);

            std::vector<String> templates = {
                "civilian_neutral", "civilian_ashvattha", "civilian_vaikuntha", "civilian_yuga_striders",
                "guard_ashvattha", "guard_vaikuntha", "guard_yuga_striders",
                "merchant_neutral", "scholar_ashvattha"
            };

            for (U32 i = 0; i < count; ++i) {
                NPCSpawnParams params;
                params.npcId = "random_npc_" + std::to_string(i);
                params.templateId = templates[i % templates.size()];

                // Random position within radius using proper distribution
                F32 angle = Math::Random::Range(0.0f, Math::Constants::TwoPiF);
                F32 distance = Math::Random::Range(0.0f, radius);
                params.position = center + Math::Vector3(
                    Math::Util::CosDegrees(angle) * distance,
                    0.0f, // Keep NPCs on ground level
                    Math::Util::SinDegrees(angle) * distance
                );

                // Random Y rotation (facing direction)
                params.rotation = Math::Vector3(0.0f, Math::Random::Range(0.0f, Math::Constants::TwoPiF), 0.0f);

                // Default scale
                params.scale = Math::Vector3::One;

                // All NPCs start active and visible
                params.isActive = true;
                params.isVisible = true;

                spawnList.push_back(params);
            }

            AGK_INFO("NPCManagerUtils: Generated {0} random NPC spawn parameters", count);
            return spawnList;
        }

        NPCSpawnParams CreateSpawnParams(const String& npcId, const String& templateId, const Vector3& position) {
            NPCSpawnParams params;
            params.npcId = npcId;
            params.templateId = templateId;
            params.position = position;
            params.rotation = Vector3::Zero;
            params.scale = Vector3::One;
            params.isActive = true;
            params.isVisible = true;

            return params;
        }

        // ==================================================================================
        // Performance Helpers
        // ==================================================================================

        std::vector<NPCController*> SortNPCsByDistance(const std::vector<NPCController*>& npcs, const Vector3& referencePoint) {
            std::vector<NPCController*> sorted = npcs;

            std::sort(sorted.begin(), sorted.end(),
                [&referencePoint](const NPCController* a, const NPCController* b) {
                    if (!a || !b) return false;
                    F32 distA = NPCUtils::CalculateDistance(a->GetPosition(), referencePoint);
                    F32 distB = NPCUtils::CalculateDistance(b->GetPosition(), referencePoint);
                    return distA < distB;
                });

            return sorted;
        }

        std::vector<NPCController*> FilterNPCsByRange(const std::vector<NPCController*>& npcs, const Vector3& center, F32 range) {
            std::vector<NPCController*> filtered;
            filtered.reserve(npcs.size()); // Reserve space for efficiency

            for (NPCController* npc : npcs) {
                if (!npc) continue;

                F32 distance = NPCUtils::CalculateDistance(npc->GetPosition(), center);
                if (distance <= range) {
                    filtered.push_back(npc);
                }
            }

            return filtered;
        }

        // ==================================================================================
        // Configuration Helpers
        // ==================================================================================

        bool SaveTemplatesToFile(const std::unordered_map<String, NPCTemplate>& templates, const String& filePath) {
            AGK_INFO("NPCManagerUtils: Saving {0} templates to file '{1}'", templates.size(), filePath);

            try {
                std::ofstream file(filePath);
                if (!file.is_open()) {
                    AGK_ERROR("NPCManagerUtils: Failed to open file '{0}' for writing", filePath);
                    return false;
                }

                // Write YAML-style template data
                file << "# NPC Templates Configuration\n";
                file << "# Generated by Angaraka Engine NPCManager\n";
                file << "templates:\n";

                for (const auto& [templateId, templ] : templates) {
                    file << "  - id: \"" << templateId << "\"\n";
                    file << "    display_name: \"" << templ.displayName << "\"\n";
                    file << "    faction: " << static_cast<int>(templ.faction) << "\n";
                    file << "    npc_type: " << static_cast<int>(templ.npcType) << "\n";
                    file << "    interaction_range: " << templ.interactionRange << "\n";
                    file << "    default_interaction: " << static_cast<int>(templ.defaultInteraction) << "\n";

                    // Resources
                    file << "    mesh_resource: \"" << templ.meshResourceId << "\"\n";
                    file << "    texture_resource: \"" << templ.textureResourceId << "\"\n";
                    file << "    dialogue_model: \"" << templ.dialogueModelId << "\"\n";
                    file << "    behavior_model: \"" << templ.behaviorModelId << "\"\n";

                    // Personality
                    file << "    personality:\n";
                    file << "      aggressiveness: " << templ.personality.aggressiveness << "\n";
                    file << "      curiosity: " << templ.personality.curiosity << "\n";
                    file << "      trustfulness: " << templ.personality.trustfulness << "\n";
                    file << "      helpfulness: " << templ.personality.helpfulness << "\n";
                    file << "      intelligence: " << templ.personality.intelligence << "\n";
                    file << "      loyalty: " << templ.personality.loyalty << "\n";

                    // Known topics
                    if (!templ.knownTopics.empty()) {
                        file << "    known_topics:\n";
                        for (const String& topic : templ.knownTopics) {
                            file << "      - \"" << topic << "\"\n";
                        }
                    }

                    // Topic expertise
                    if (!templ.topicExpertise.empty()) {
                        file << "    topic_expertise:\n";
                        for (const auto& [topic, expertise] : templ.topicExpertise) {
                            file << "      \"" << topic << "\": " << expertise << "\n";
                        }
                    }

                    // Description
                    if (!templ.description.empty()) {
                        file << "    description: \"" << templ.description << "\"\n";
                    }

                    file << "\n"; // Separator between templates
                }

                file.close();
                AGK_INFO("NPCManagerUtils: Successfully saved {0} templates to '{1}'", templates.size(), filePath);
                return true;
            }
            catch (const std::exception& e) {
                AGK_ERROR("NPCManagerUtils: Exception while saving templates: {0}", e.what());
                return false;
            }
        }

        bool LoadTemplatesFromFile(std::unordered_map<String, NPCTemplate>& templates, const String& filePath) {
            AGK_INFO("NPCManagerUtils: Loading templates from file '{0}'", filePath);

            try {
                std::ifstream file(filePath);
                if (!file.is_open()) {
                    AGK_ERROR("NPCManagerUtils: Failed to open file '{0}' for reading", filePath);
                    return false;
                }

                // Note: This is a simplified YAML parser for demonstration
                // In a production environment, you'd use a proper YAML library like yaml-cpp

                String line;
                NPCTemplate* currentTemplate = nullptr;
                U32 loadedCount = 0;

                while (std::getline(file, line)) {
                    // Trim whitespace
                    line.erase(0, line.find_first_not_of(" \t"));
                    line.erase(line.find_last_not_of(" \t") + 1);

                    // Skip comments and empty lines
                    if (line.empty() || line[0] == '#') {
                        continue;
                    }

                    // Parse template start
                    if (line.find("- id:") != String::npos) {
                        // Extract template ID
                        size_t quoteStart = line.find('"');
                        size_t quoteEnd = line.find('"', quoteStart + 1);
                        if (quoteStart != String::npos && quoteEnd != String::npos) {
                            String templateId = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);

                            // Create new template
                            templates[templateId] = NPCTemplate(templateId, "Loaded Template", NPCFaction::Neutral);
                            currentTemplate = &templates[templateId];
                            loadedCount++;
                        }
                    }
                    else if (currentTemplate && line.find("display_name:") != String::npos) {
                        size_t quoteStart = line.find('"');
                        size_t quoteEnd = line.find('"', quoteStart + 1);
                        if (quoteStart != String::npos && quoteEnd != String::npos) {
                            currentTemplate->displayName = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                        }
                    }
                    else if (currentTemplate && line.find("faction:") != String::npos) {
                        size_t colonPos = line.find(':');
                        if (colonPos != String::npos) {
                            int factionValue = std::stoi(line.substr(colonPos + 1));
                            currentTemplate->faction = static_cast<NPCFaction>(factionValue);
                        }
                    }
                    else if (currentTemplate && line.find("interaction_range:") != String::npos) {
                        size_t colonPos = line.find(':');
                        if (colonPos != String::npos) {
                            currentTemplate->interactionRange = std::stof(line.substr(colonPos + 1));
                        }
                    }
                    // Add more parsing logic as needed for other fields...
                }

                file.close();
                AGK_INFO("NPCManagerUtils: Successfully loaded {0} templates from '{1}'", loadedCount, filePath);
                return loadedCount > 0;
            }
            catch (const std::exception& e) {
                AGK_ERROR("NPCManagerUtils: Exception while loading templates: {0}", e.what());
                return false;
            }
        }

        // ==================================================================================
        // Debug Helpers
        // ==================================================================================

        String GetNPCStatusSummary(const std::vector<NPCController*>& npcs) {
            std::unordered_map<NPCState, U32> stateCounts;
            std::unordered_map<NPCFaction, U32> factionCounts;
            std::unordered_map<NPCType, U32> typeCounts;

            U32 activeCount = 0;
            U32 visibleCount = 0;
            U32 interactableCount = 0;

            for (const NPCController* npc : npcs) {
                if (!npc) continue;

                const NPCComponent& npcData = npc->GetNPCData();

                // Count by state
                stateCounts[npcData.currentState]++;

                // Count by faction
                factionCounts[npcData.faction]++;

                // Count by type
                typeCounts[npcData.npcType]++;

                // Count status flags
                if (npc->IsActive()) activeCount++;
                if (npcData.isVisible) visibleCount++;
                if (npc->CanInteractWithPlayer()) interactableCount++;
            }

            std::stringstream summary;
            summary << "=== NPC Status Summary ===\n";
            summary << "Total NPCs: " << npcs.size() << "\n";
            summary << "Active: " << activeCount << ", Visible: " << visibleCount << ", Interactable: " << interactableCount << "\n\n";

            // State breakdown
            summary << "States:\n";
            for (const auto& [state, count] : stateCounts) {
                const char* stateName = "Unknown";
                switch (state) {
                case NPCState::Idle: stateName = "Idle"; break;
                case NPCState::Patrol: stateName = "Patrol"; break;
                case NPCState::Conversation: stateName = "Conversation"; break;
                case NPCState::Alert: stateName = "Alert"; break;
                case NPCState::Working: stateName = "Working"; break;
                case NPCState::Sleeping: stateName = "Sleeping"; break;
                case NPCState::Following: stateName = "Following"; break;
                case NPCState::Hostile: stateName = "Hostile"; break;
                default: break;
                }
                summary << "  " << stateName << ": " << count << "\n";
            }

            // Faction breakdown
            summary << "\nFactions:\n";
            for (const auto& [faction, count] : factionCounts) {
                const char* factionName = "Unknown";
                switch (faction) {
                case NPCFaction::Neutral: factionName = "Neutral"; break;
                case NPCFaction::Ashvattha: factionName = "Ashvattha"; break;
                case NPCFaction::Vaikuntha: factionName = "Vaikuntha"; break;
                case NPCFaction::YugaStriders: factionName = "Yuga Striders"; break;
                default: break;
                }
                summary << "  " << factionName << ": " << count << "\n";
            }

            // Type breakdown
            summary << "\nTypes:\n";
            for (const auto& [type, count] : typeCounts) {
                const char* typeName = "Unknown";
                switch (type) {
                case NPCType::Civilian: typeName = "Civilian"; break;
                case NPCType::Guard: typeName = "Guard"; break;
                case NPCType::Merchant: typeName = "Merchant"; break;
                case NPCType::Scholar: typeName = "Scholar"; break;
                case NPCType::Leader: typeName = "Leader"; break;
                case NPCType::Worker: typeName = "Worker"; break;
                default: break;
                }
                summary << "  " << typeName << ": " << count << "\n";
            }

            return summary.str();
        }

        void LogNPCDistribution(const std::vector<NPCController*>& npcs, const Vector3& playerPos) {
            if (npcs.empty()) {
                AGK_INFO("NPCManagerUtils: No NPCs to analyze");
                return;
            }

            F32 minDistance = std::numeric_limits<F32>::max();
            F32 maxDistance = 0.0f;
            F32 totalDistance = 0.0f;

            for (const NPCController* npc : npcs) {
                if (!npc) continue;

                F32 distance = NPCUtils::CalculateDistance(npc->GetPosition(), playerPos);
                minDistance = Math::Util::Min(minDistance, distance);
                maxDistance = Math::Util::Max(maxDistance, distance);
                totalDistance += distance;
            }

            F32 averageDistance = totalDistance / npcs.size();

            AGK_INFO("NPCManagerUtils: NPC Distribution - Count: {0}, Min: {1:.1f}, Max: {2:.1f}, Avg: {3:.1f}",
                npcs.size(), minDistance, maxDistance, averageDistance);

            // Distance range analysis
            U32 closeCount = 0;      // < 10 units
            U32 nearCount = 0;       // 10-50 units
            U32 farCount = 0;        // 50-100 units
            U32 distantCount = 0;    // > 100 units

            for (const NPCController* npc : npcs) {
                if (!npc) continue;

                F32 distance = NPCUtils::CalculateDistance(npc->GetPosition(), playerPos);
                if (distance < 10.0f) closeCount++;
                else if (distance < 50.0f) nearCount++;
                else if (distance < 100.0f) farCount++;
                else distantCount++;
            }

            AGK_INFO("NPCManagerUtils: Distance Ranges - Close(<10): {0}, Near(10-50): {1}, Far(50-100): {2}, Distant(>100): {3}",
                closeCount, nearCount, farCount, distantCount);

            // Log detailed breakdown by faction
            std::unordered_map<NPCFaction, std::vector<F32>> factionDistances;
            for (const NPCController* npc : npcs) {
                if (!npc) continue;

                F32 distance = NPCUtils::CalculateDistance(npc->GetPosition(), playerPos);
                factionDistances[npc->GetNPCData().faction].push_back(distance);
            }

            for (const auto& [faction, distances] : factionDistances) {
                if (distances.empty()) continue;

                F32 factionTotal = 0.0f;
                for (F32 dist : distances) {
                    factionTotal += dist;
                }
                F32 factionAvg = factionTotal / distances.size();

                const char* factionName = "Unknown";
                switch (faction) {
                case NPCFaction::Neutral: factionName = "Neutral"; break;
                case NPCFaction::Ashvattha: factionName = "Ashvattha"; break;
                case NPCFaction::Vaikuntha: factionName = "Vaikuntha"; break;
                case NPCFaction::YugaStriders: factionName = "Yuga Striders"; break;
                default: break;
                }

                AGK_DEBUG("NPCManagerUtils: {0} faction - {1} NPCs, Avg distance: {2:.1f}",
                    factionName, distances.size(), factionAvg);
            }
        }
    }
}