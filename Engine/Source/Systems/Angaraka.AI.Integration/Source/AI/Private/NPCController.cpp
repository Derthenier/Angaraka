#include "Angaraka/NPCController.hpp"
#include <Angaraka/AIManager.hpp>
#include <sstream>

import Angaraka.Math;
import Angaraka.Math.Random;

import Angaraka.Core.Events;
import Angaraka.Core.ResourceCache;
import Angaraka.Graphics.DirectX12;

using namespace Angaraka::Core;
using namespace Angaraka::Events;

namespace Angaraka::AI {

    // ==================================================================================
    // Constructor and Destructor
    // ==================================================================================

    NPCController::NPCController(
        const String& npcId,
        Reference<CachedResourceManager> resourceManager,
        Reference<AIManager> aiManager,
        Reference<DirectX12GraphicsSystem> graphicsSystem)
        : m_npcData(npcId, "DefaultNPC")
        , m_resourceManager(resourceManager)
        , m_aiManager(aiManager)
        , m_graphicsSystem(graphicsSystem)
        , m_isInitialized(false)
        , m_pendingAIDecision(false)
        , m_lastUpdate(std::chrono::steady_clock::now())
        , m_totalUpdateTime(0.0f)
        , m_updateCount(0)
    {
        AGK_INFO("NPCController: Creating NPC '{0}'", npcId);
    }

    NPCController::~NPCController() {
        AGK_INFO("NPCController: Destroying NPC '{0}'", m_npcData.npcId);
        Shutdown();
    }

    // ==================================================================================
    // Core Lifecycle
    // ==================================================================================

    bool NPCController::Initialize(const NPCComponent& initialData) {
        if (m_isInitialized) {
            AGK_WARN("NPCController: NPC '{0}' already initialized", m_npcData.npcId);
            return true;
        }

        AGK_INFO("NPCController: Initializing NPC '{0}' (Faction: {1})",
            initialData.npcId, initialData.GetFactionString());

        // Copy initial data
        m_npcData = initialData;

        // Validate required systems
        if (!m_resourceManager || !m_aiManager || !m_graphicsSystem) {
            AGK_ERROR("NPCController: Missing required system dependencies for NPC '{0}'", m_npcData.npcId);
            return false;
        }

        // Validate NPC data
        ValidateNPCData();

        // Subscribe to events
        SubscribeToEvents();

        // Load resources if specified
        if (!m_npcData.meshResourceId.empty()) {
            SetMeshResource(m_npcData.meshResourceId);
        }

        if (!m_npcData.textureResourceId.empty()) {
            SetTextureResource(m_npcData.textureResourceId);
        }

        // Initialize AI state
        m_npcData.MarkAIUpdated();
        m_lastUpdate = std::chrono::steady_clock::now();

        m_isInitialized = true;
        AGK_INFO("NPCController: Successfully initialized NPC '{0}'", m_npcData.npcId);

        // Trigger initial state setup
        ChangeState(m_npcData.currentState, "Initialization");

        return true;
    }

    void NPCController::Update(F32 deltaTime) {
        if (!m_isInitialized || !m_npcData.isActive) {
            return;
        }

        auto updateStart = std::chrono::steady_clock::now();

        // Performance optimization - skip update if too far from player
        if (ShouldSkipUpdate()) {
            return;
        }

        // Update AI behavior at intervals
        if (m_npcData.ShouldUpdateAI()) {
            UpdateAIBehavior(deltaTime);
            m_npcData.MarkAIUpdated();
        }

        // State-specific behavior updates
        switch (m_npcData.currentState) {
        case NPCState::Idle:
            UpdateIdleBehavior(deltaTime);
            break;
        case NPCState::Conversation:
            UpdateConversationBehavior(deltaTime);
            break;
        case NPCState::Patrol:
            UpdatePatrolBehavior(deltaTime);
            break;
        case NPCState::Alert:
            UpdateAlertBehavior(deltaTime);
            break;
        default:
            // Other states can be handled by custom callbacks
            break;
        }

        // Execute custom update callback if set
        if (m_updateCallback) {
            m_updateCallback(m_npcData, deltaTime);
        }

        // Update performance metrics
        UpdatePerformanceMetrics(deltaTime);

        m_lastUpdate = std::chrono::steady_clock::now();
    }

    void NPCController::Shutdown() {
        if (!m_isInitialized) {
            return;
        }

        AGK_INFO("NPCController: Shutting down NPC '{0}'", m_npcData.npcId);

        // Unsubscribe from events
        UnsubscribeFromEvents();

        // Clear callbacks
        m_updateCallback = nullptr;
        m_interactionCallback = nullptr;
        m_stateChangeCallback = nullptr;

        // Mark as inactive
        m_npcData.isActive = false;
        m_isInitialized = false;

        AGK_INFO("NPCController: NPC '{0}' shutdown complete", m_npcData.npcId);
    }

    // ==================================================================================
    // State Management
    // ==================================================================================

    void NPCController::ChangeState(NPCState newState, const String& reason) {
        if (m_npcData.currentState == newState) {
            return; // Already in the target state
        }

        if (!CanChangeState(newState)) {
            AGK_WARN("NPCController: Invalid state transition from {0} to {1} for NPC '{2}'",
                m_npcData.GetStateString(), (int)newState, m_npcData.npcId);
            return;
        }

        NPCState oldState = m_npcData.currentState;

        if (ValidateStateTransition(oldState, newState)) {
            m_npcData.previousState = oldState;
            m_npcData.currentState = newState;

            LogStateTransition(oldState, newState, reason);
            OnStateChanged(oldState, newState);

            // Publish state change event

            NPCStateChangeEvent stateEvent;
            stateEvent.npcId = m_npcData.npcId;
            stateEvent.oldState = oldState;
            stateEvent.newState = newState;
            stateEvent.reason = reason;

            // Note: Actual event publishing would depend on your EventManager API
            // m_eventManager->Publish(stateEvent);
            Angaraka::Events::EventManager::Get().Broadcast(stateEvent);
            AGK_INFO("NPCController: Publishing state change event for NPC '{0}'", m_npcData.npcId);
        }
    }

    bool NPCController::CanChangeState(NPCState newState) const {
        // Basic validation - can be extended with more complex rules
        switch (m_npcData.currentState) {
        case NPCState::Conversation:
            // Can only exit conversation to idle or alert
            return newState == NPCState::Idle || newState == NPCState::Alert;

        case NPCState::Sleeping:
            // Need external trigger to wake up
            return newState == NPCState::Idle || newState == NPCState::Alert;

        case NPCState::Hostile:
            // Once hostile, can only go to alert or back to previous peaceful state
            return newState == NPCState::Alert || newState == NPCState::Idle;

        default:
            return true; // Most states can transition freely
        }
    }

    // ==================================================================================
    // Position and World Interaction
    // ==================================================================================

    void NPCController::SetPosition(const Vector3& position) {
        m_npcData.position = position;
        AGK_DEBUG("NPCController: Set position for NPC '{0}' to ({1}, {2}, {3})",
            m_npcData.npcId, position.x, position.y, position.z);
    }

    void NPCController::SetRotation(const Vector3& rotation) {
        m_npcData.rotation = rotation;
        AGK_DEBUG("NPCController: Set rotation for NPC '{0}' to ({1}, {2}, {3})",
            m_npcData.npcId, rotation.x, rotation.y, rotation.z);
    }

    bool NPCController::IsPlayerInRange(const Vector3& playerPosition) const {
        F32 distance = NPCUtils::CalculateDistance(m_npcData.position, playerPosition);
        return distance <= m_npcData.interactionRange;
    }

    bool NPCController::CanInteractWithPlayer() const {
        return m_npcData.isInteractable &&
            m_npcData.isActive &&
            NPCUtils::IsInteractiveState(m_npcData.currentState);
    }

    void NPCController::TriggerInteraction(const NPCInteractionEvent& interaction) {
        if (!CanInteractWithPlayer()) {
            AGK_WARN("NPCController: Cannot interact with NPC '{0}' in current state", m_npcData.npcId);
            return;
        }

        AGK_INFO("NPCController: Player interaction triggered for NPC '{0}' (Action: {1})",
            m_npcData.npcId, interaction.playerAction);

        // Update relationship based on player action
        ProcessPlayerAction(interaction.playerAction);

        // Execute custom interaction callback
        if (m_interactionCallback) {
            m_interactionCallback(m_npcData, interaction);
        }

        // Handle different interaction types
        switch (interaction.interactionType) {
        case InteractionType::Dialogue:
            ChangeState(NPCState::Conversation, "Player initiated dialogue");
            ProcessDialogueRequest(interaction.playerAction, BuildConversationContext());
            break;

        case InteractionType::Trade:
            ChangeState(NPCState::Conversation, "Player initiated trade");
            // Trade logic would go here
            break;

        case InteractionType::Hostile:
            ChangeState(NPCState::Hostile, "Player hostile action");
            break;

        default:
            AGK_INFO("NPCController: Unhandled interaction type for NPC '{0}'", m_npcData.npcId);
            break;
        }

        // Update interaction tracking
        m_npcData.relationship.conversationCount++;
        m_npcData.relationship.lastInteraction = std::chrono::steady_clock::now();
    }

    // ==================================================================================
    // AI Behavior Integration
    // ==================================================================================

    void NPCController::RequestAIDecision(const String& situation, const std::vector<String>& availableActions) {
        if (!m_aiManager || m_pendingAIDecision) {
            return;
        }

        AGK_INFO("NPCController: Requesting AI decision for NPC '{0}' (Situation: {1})",
            m_npcData.npcId, situation);

        m_pendingAIDecision = true;
        m_currentAISituation = situation;
        m_currentAIActions = availableActions;

        // Prepare behavior request
        BehaviorRequest request;
        request.factionId = m_npcData.GetFactionString();
        request.npcType = "civilian"; // Could be made configurable
        request.currentSituation = situation;
        request.availableActions = availableActions;
        request.worldState = GetCurrentWorldState();
        request.timeConstraint = 1.0f; // Normal priority

        // Request decision from AI system (async)
        auto response = m_aiManager->EvaluateBehaviorSync(request);

        if (response.success) {
            OnAIDecisionResponse(m_npcData.npcId, response.selectedAction, response.reasoning);
        }
        else {
            AGK_ERROR("NPCController: AI decision request failed for NPC '{0}'", m_npcData.npcId);
            m_pendingAIDecision = false;
        }
    }

    void NPCController::ProcessDialogueRequest(const String& playerMessage, const String& context) {
        if (!m_aiManager) {
            AGK_ERROR("NPCController: No AI manager available for dialogue processing");
            return;
        }

        AGK_INFO("NPCController: Processing dialogue request for NPC '{0}'", m_npcData.npcId);

        // Prepare dialogue request
        DialogueRequest request;
        request.factionId = m_npcData.GetFactionString();
        request.npcId = m_npcData.npcId;
        request.playerMessage = playerMessage;
        request.conversationContext = context;

        // Add emotional state
        request.emotionalState["trust"] = m_npcData.relationship.trust;
        request.emotionalState["respect"] = m_npcData.relationship.respect;
        request.emotionalState["fear"] = m_npcData.relationship.fear;

        // Add recent events
        request.recentEvents = m_npcData.knowledge.recentEvents;
        request.urgency = 0.5f; // Normal conversation urgency

        // Request dialogue from AI system (async)
        auto response = m_aiManager->GenerateDialogueSync(request);

        if (response.success) {
            OnDialogueResponse(m_npcData.npcId, response.response, response.emotionalTone);
        }
        else {
            AGK_ERROR("NPCController: Dialogue generation failed for NPC '{0}'", m_npcData.npcId);
        }
    }

    // ==================================================================================
    // Resource Management
    // ==================================================================================

    void NPCController::SetMeshResource(const String& meshResourceId) {
        if (!m_resourceManager) {
            AGK_ERROR("NPCController: No resource manager available");
            return;
        }

        m_npcData.meshResourceId = meshResourceId;

        // Load mesh resource through CachedResourceManager
        // Note: Actual resource loading would depend on your ResourceManager API
        AGK_INFO("NPCController: Loading mesh resource '{0}' for NPC '{1}'",
            meshResourceId, m_npcData.npcId);
    }

    void NPCController::SetTextureResource(const String& textureResourceId) {
        if (!m_resourceManager) {
            AGK_ERROR("NPCController: No resource manager available");
            return;
        }

        m_npcData.textureResourceId = textureResourceId;

        // Load texture resource through CachedResourceManager
        AGK_INFO("NPCController: Loading texture resource '{0}' for NPC '{1}'",
            textureResourceId, m_npcData.npcId);
    }

    // ==================================================================================
    // Relationship Management
    // ==================================================================================

    void NPCController::UpdateRelationship(F32 trustDelta, F32 respectDelta, F32 fearDelta) {
        m_npcData.relationship.trust = Math::Util::Clamp(m_npcData.relationship.trust + trustDelta, 0.0f, 1.0f);
        m_npcData.relationship.respect = Math::Util::Clamp(m_npcData.relationship.respect + respectDelta, 0.0f, 1.0f);
        m_npcData.relationship.fear = Math::Util::Clamp(m_npcData.relationship.fear + fearDelta, 0.0f, 1.0f);

        AGK_DEBUG("NPCController: Updated relationship for NPC '{0}' - Trust: {1:.2f}, Respect: {2:.2f}, Fear: {3:.2f}",
            m_npcData.npcId, m_npcData.relationship.trust, m_npcData.relationship.respect, m_npcData.relationship.fear);
    }

    // ==================================================================================
    // Knowledge and Memory
    // ==================================================================================

    void NPCController::AddKnownTopic(const String& topic, F32 expertise) {
        auto it = std::find(m_npcData.knowledge.knownTopics.begin(), m_npcData.knowledge.knownTopics.end(), topic);
        if (it == m_npcData.knowledge.knownTopics.end()) {
            m_npcData.knowledge.knownTopics.push_back(topic);
        }

        m_npcData.knowledge.topicExpertise[topic] = Math::Util::Clamp(expertise, 0.0f, 1.0f);

        AGK_DEBUG("NPCController: Added topic '{0}' with expertise {1:.2f} to NPC '{2}'",
            topic, expertise, m_npcData.npcId);
    }

    void NPCController::RecordEvent(const String& event) {
        m_npcData.knowledge.recentEvents.push_back(event);

        // Keep only recent events (last 10)
        if (m_npcData.knowledge.recentEvents.size() > 10) {
            m_npcData.knowledge.recentEvents.erase(m_npcData.knowledge.recentEvents.begin());
        }

        AGK_DEBUG("NPCController: Recorded event '{0}' for NPC '{1}'", event, m_npcData.npcId);
    }

    bool NPCController::HasKnowledge(const String& topic) const {
        auto it = std::find(m_npcData.knowledge.knownTopics.begin(), m_npcData.knowledge.knownTopics.end(), topic);
        return it != m_npcData.knowledge.knownTopics.end();
    }

    // ==================================================================================
    // Performance Optimization
    // ==================================================================================

    void NPCController::SetActive(bool active) {
        m_npcData.isActive = active;
        AGK_DEBUG("NPCController: Set NPC '{0}' active state to {1}", m_npcData.npcId, active);
    }

    void NPCController::UpdateDistanceToPlayer(const Vector3& playerPosition) {
        m_npcData.distanceToPlayer = NPCUtils::CalculateDistance(m_npcData.position, playerPosition);

        // Update visibility based on distance and other factors
        // This is a simple implementation - could be more sophisticated
        m_npcData.isInPlayerView = m_npcData.distanceToPlayer <= 100.0f; // 100 unit view distance
    }

    // ==================================================================================
    // Debug and Diagnostics
    // ==================================================================================

    String NPCController::GetDebugInfo() const {
        std::stringstream ss;
        ss << "=== NPC Debug Info: " << m_npcData.npcId << " ===\n";
        ss << "State: " << m_npcData.GetStateString() << "\n";
        ss << "Faction: " << m_npcData.GetFactionString() << "\n";
        ss << "Position: (" << m_npcData.position.x << ", " << m_npcData.position.y << ", " << m_npcData.position.z << ")\n";
        ss << "Distance to Player: " << m_npcData.distanceToPlayer << "\n";
        ss << "Relationship - Trust: " << m_npcData.relationship.trust << ", Respect: " << m_npcData.relationship.respect << "\n";
        ss << "Update Count: " << m_updateCount << ", Avg Update Time: " << (m_updateCount > 0 ? m_totalUpdateTime / m_updateCount : 0.0f) << "ms\n";
        ss << "AI Models - Dialogue: " << m_npcData.dialogueModelId << ", Behavior: " << m_npcData.behaviorModelId << "\n";
        ss << "Known Topics: " << m_npcData.knowledge.knownTopics.size() << ", Recent Events: " << m_npcData.knowledge.recentEvents.size();

        return ss.str();
    }

    void NPCController::LogCurrentState() const {
        AGK_INFO("NPCController State - NPC: '{0}', State: {1}, Active: {2}, Distance: {3:.2f}",
            m_npcData.npcId, m_npcData.GetStateString(), m_npcData.isActive, m_npcData.distanceToPlayer);
    }

    // ==================================================================================
    // Private Helper Methods
    // ==================================================================================

    void NPCController::SubscribeToEvents() {
        // Note: Actual event subscription would depend on your EventManager API
        // Example subscriptions:
        // Angaraka::Events::EventManager::Get().Subscribe<PlayerMovementEvent>([this](const PlayerMovementEvent& e) { OnPlayerMovement(e.position); }));

        AGK_INFO("NPCController: Subscribed to events for NPC '{0}'", m_npcData.npcId);
    }

    void NPCController::UnsubscribeFromEvents() {
        // Note: Actual event unsubscription would depend on your EventManager API
        AGK_INFO("NPCController: Unsubscribed from events for NPC '{0}'", m_npcData.npcId);
    }

    void NPCController::OnPlayerMovement(const Vector3& playerPosition) {
        UpdateDistanceToPlayer(playerPosition);

        // React to player proximity
        if (IsPlayerInRange(playerPosition) && m_npcData.currentState == NPCState::Idle) {
            // Could trigger alert state or other behaviors
            AGK_DEBUG("NPCController: Player in range of NPC '{0}'", m_npcData.npcId);
        }
    }

    void NPCController::OnPlayerInteraction(const NPCInteractionEvent& interaction) {
        if (interaction.npcId == m_npcData.npcId) {
            TriggerInteraction(interaction);
        }
    }

    void NPCController::OnAIDecisionResponse(const String& npcId, const String& selectedAction, const String& reasoning) {
        if (npcId != m_npcData.npcId) {
            return;
        }

        AGK_INFO("NPCController: AI decision received for NPC '{0}' - Action: {1}, Reasoning: {2}",
            npcId, selectedAction, reasoning);

        m_pendingAIDecision = false;

        // Process the AI decision
        if (selectedAction == "approach_player") {
            ChangeState(NPCState::Patrol, "AI decided to approach player");
        }
        else if (selectedAction == "start_conversation") {
            ChangeState(NPCState::Conversation, "AI decided to start conversation");
        }
        else if (selectedAction == "become_alert") {
            ChangeState(NPCState::Alert, "AI decided to become alert");
        }
        else if (selectedAction == "continue_idle") {
            // Stay in current state
        }
        else {
            AGK_WARN("NPCController: Unknown AI action '{0}' for NPC '{1}'", selectedAction, npcId);
        }

        // Record the AI reasoning as knowledge
        RecordEvent("AI Decision: " + reasoning);
    }

    void NPCController::OnDialogueResponse(const String& npcId, const String& response, const String& emotionalTone) {
        if (npcId != m_npcData.npcId) {
            return;
        }

        AGK_INFO("NPCController: Dialogue response received for NPC '{0}' - Tone: {1}", npcId, emotionalTone);

        // Publish dialogue response event for UI system

        NPCDialogueResponseEvent dialogueEvent;
        dialogueEvent.npcId = npcId;
        dialogueEvent.npcResponse = response;
        dialogueEvent.emotionalTone = emotionalTone;
        dialogueEvent.endConversation = false; // Could be determined by AI
        Angaraka::Events::EventManager::Get().Broadcast(dialogueEvent);

        // Note: Actual event publishing would depend on your EventManager API
        AGK_INFO("NPCController: Publishing dialogue response event for NPC '{0}'", npcId);

        // Update NPC knowledge with conversation content
        RecordEvent("Said: " + response);
    }

    bool NPCController::ValidateStateTransition(NPCState fromState, NPCState toState) const {
        // Basic validation - extend as needed
        if (fromState == toState) {
            return false; // No transition needed
        }

        // Add specific validation rules here
        return true;
    }

    void NPCController::OnStateChanged(NPCState oldState, NPCState newState) {
        // Execute state change callback
        if (m_stateChangeCallback) {
            m_stateChangeCallback(m_npcData, oldState, newState);
        }

        // State-specific initialization
        switch (newState) {
        case NPCState::Conversation:
            // Prepare for conversation
            m_npcData.knowledge.lastPlayerAction = "Started conversation";
            break;

        case NPCState::Alert:
            // Increase awareness
            RecordEvent("Became alert");
            break;

        case NPCState::Hostile:
            // Prepare for combat or defensive behavior
            UpdateRelationship(-0.2f, -0.1f, 0.3f); // Decrease trust/respect, increase fear
            break;

        default:
            break;
        }
    }

    void NPCController::UpdateAIBehavior(F32 deltaTime) {
        // Determine current situation
        String situation = "normal";
        std::vector<String> availableActions = { "continue_idle", "patrol_area", "become_alert" };

        // Modify based on player proximity
        if (m_npcData.distanceToPlayer < m_npcData.interactionRange) {
            situation = "player_nearby";
            availableActions = { "approach_player", "start_conversation", "become_alert", "continue_idle" };
        }

        // Modify based on faction relationships
        if (m_npcData.relationship.fear > 0.7f) {
            situation = "fearful";
            availableActions = { "become_alert", "flee", "cower" };
        }
        else if (m_npcData.relationship.trust > 0.8f) {
            situation = "trusting";
            availableActions = { "approach_player", "start_conversation", "offer_help" };
        }

        // Request AI decision if not already pending
        if (!m_pendingAIDecision) {
            RequestAIDecision(situation, availableActions);
        }
    }

    void NPCController::UpdateIdleBehavior(F32 deltaTime) {
        // Simple idle behavior - could be extended
        static F32 idleTimer = 0.0f;
        idleTimer += deltaTime;

        if (idleTimer > 5.0f) { // Every 5 seconds
            idleTimer = 0.0f;

            // Randomly look around or perform idle actions
            if (m_npcData.distanceToPlayer > m_npcData.interactionRange * 2.0f) {
                // Far from player - might start patrolling
                if (Math::Random::Range(0.0f, 1.0f) < 0.1f) { // 10% chance per check
                    ChangeState(NPCState::Patrol, "Random patrol decision");
                }
            }
        }
    }

    void NPCController::UpdateConversationBehavior(F32 deltaTime) {
        // Handle conversation timeout
        static F32 conversationTimer = 0.0f;
        conversationTimer += deltaTime;

        if (conversationTimer > 30.0f) { // 30 second conversation timeout
            conversationTimer = 0.0f;
            ChangeState(NPCState::Idle, "Conversation timeout");
        }

        // Check if player has moved too far away
        if (m_npcData.distanceToPlayer > m_npcData.interactionRange * 1.5f) {
            ChangeState(NPCState::Idle, "Player moved away");
        }
    }

    void NPCController::UpdatePatrolBehavior(F32 deltaTime) {
        // Simple patrol behavior - move around slightly
        static F32 patrolTimer = 0.0f;
        patrolTimer += deltaTime;

        if (patrolTimer > 3.0f) { // Change direction every 3 seconds
            patrolTimer = 0.0f;

            // Simple random movement (could be enhanced with waypoints)
            Vector3 randomOffset = Vector3(
                Math::Random::Range(-2.0f, 2.0f),
                0.0f,
                Math::Random::Range(-2.0f, 2.0f)
            );

            SetPosition(m_npcData.position + randomOffset);

            // Return to idle after some time
            if (Math::Random::Range(0.0f, 1.0f) < 0.3f) { // 30% chance to stop patrolling
                ChangeState(NPCState::Idle, "Finished patrolling");
            }
        }
    }

    void NPCController::UpdateAlertBehavior(F32 deltaTime) {
        // Alert behavior - heightened awareness
        static F32 alertTimer = 0.0f;
        alertTimer += deltaTime;

        if (alertTimer > 10.0f) { // Stay alert for 10 seconds
            alertTimer = 0.0f;
            ChangeState(NPCState::Idle, "Alert timeout");
        }

        // Check for threats or interesting events
        if (m_npcData.distanceToPlayer < m_npcData.interactionRange * 0.5f) {
            // Player very close - might start conversation or become hostile
            if (m_npcData.relationship.fear > 0.5f) {
                ChangeState(NPCState::Hostile, "Player too close while afraid");
            }
            else if (m_npcData.relationship.trust > 0.6f) {
                ChangeState(NPCState::Conversation, "Trusted player approached");
            }
        }
    }

    String NPCController::BuildConversationContext() const {
        std::stringstream context;
        context << "NPC: " << m_npcData.displayName;
        context << ", Faction: " << m_npcData.GetFactionString();
        context << ", Relationship - Trust: " << m_npcData.relationship.trust;
        context << ", Conversations: " << m_npcData.relationship.conversationCount;

        if (!m_npcData.knowledge.recentEvents.empty()) {
            context << ", Recent: " << m_npcData.knowledge.recentEvents.back();
        }

        return context.str();
    }

    std::unordered_map<String, F32> NPCController::GetCurrentWorldState() const {
        std::unordered_map<String, F32> worldState;

        worldState["distance_to_player"] = m_npcData.distanceToPlayer;
        worldState["trust_level"] = m_npcData.relationship.trust;
        worldState["respect_level"] = m_npcData.relationship.respect;
        worldState["fear_level"] = m_npcData.relationship.fear;
        worldState["conversation_count"] = static_cast<F32>(m_npcData.relationship.conversationCount);
        worldState["time_since_last_interaction"] = 0.0f; // Calculate actual time difference
        worldState["player_in_range"] = m_npcData.distanceToPlayer <= m_npcData.interactionRange ? 1.0f : 0.0f;
        worldState["faction_loyalty"] = m_npcData.personality.loyalty;
        worldState["aggressiveness"] = m_npcData.personality.aggressiveness;

        return worldState;
    }

    void NPCController::ProcessPlayerAction(const String& action) {
        F32 relationshipModifier = CalculateRelationshipModifier(action);

        // Update relationship based on action
        if (action == "greeting" || action == "friendly_approach") {
            UpdateRelationship(0.05f, 0.02f, -0.01f);
        }
        else if (action == "aggressive" || action == "threatening") {
            UpdateRelationship(-0.1f, -0.05f, 0.15f);
        }
        else if (action == "gift" || action == "help") {
            UpdateRelationship(0.1f, 0.08f, -0.02f);
        }
        else if (action == "ignore") {
            UpdateRelationship(-0.02f, -0.01f, 0.0f);
        }

        // Record the action
        m_npcData.knowledge.lastPlayerAction = action;
        RecordEvent("Player: " + action);
    }

    F32 NPCController::CalculateRelationshipModifier(const String& action) const {
        // Base modifier affected by personality
        F32 modifier = 1.0f;

        if (action == "aggressive" || action == "threatening") {
            // Aggressive NPCs might respect intimidation, peaceful ones fear it
            modifier = m_npcData.personality.aggressiveness > 0.7f ? 0.5f : 1.5f;
        }
        else if (action == "friendly_approach" || action == "help") {
            // Trusting NPCs respond well to friendliness
            modifier = m_npcData.personality.trustfulness;
        }

        // Faction-specific modifiers
        switch (m_npcData.faction) {
        case NPCFaction::Ashvattha:
            // Traditional faction values respect and wisdom
            if (action == "respectful" || action == "philosophical") {
                modifier *= 1.5f;
            }
            break;

        case NPCFaction::Vaikuntha:
            // Corporate faction values efficiency and logic
            if (action == "logical" || action == "efficient") {
                modifier *= 1.3f;
            }
            break;

        case NPCFaction::YugaStriders:
            // Rebel faction values freedom and authenticity
            if (action == "rebellious" || action == "honest") {
                modifier *= 1.4f;
            }
            break;

        default:
            break;
        }

        return modifier;
    }

    bool NPCController::ShouldSkipUpdate() const {
        // Skip update if too far from player (performance optimization)
        const F32 MAX_UPDATE_DISTANCE = 150.0f;

        if (m_npcData.distanceToPlayer > MAX_UPDATE_DISTANCE) {
            return true;
        }

        // Skip if not visible and in idle state
        if (!m_npcData.isInPlayerView && m_npcData.currentState == NPCState::Idle) {
            return true;
        }

        return false;
    }

    void NPCController::UpdatePerformanceMetrics(F32 deltaTime) {
        m_updateCount++;
        m_totalUpdateTime += deltaTime * 1000.0f; // Convert to milliseconds

        // Log performance every 100 updates
        if (m_updateCount % 100 == 0) {
            F32 avgUpdateTime = m_totalUpdateTime / m_updateCount;
            if (avgUpdateTime > 5.0f) { // Warn if average update time > 5ms
                AGK_WARN("NPCController: High update time for NPC '{0}': {1:.2f}ms average",
                    m_npcData.npcId, avgUpdateTime);
            }
        }
    }

    void NPCController::ValidateNPCData() const {
        // Validate critical NPC data
        if (m_npcData.npcId.empty()) {
            AGK_ERROR("NPCController: NPC ID cannot be empty");
        }

        if (m_npcData.displayName.empty()) {
            AGK_WARN("NPCController: NPC '{0}' has empty display name", m_npcData.npcId);
        }

        if (m_npcData.interactionRange <= 0.0f) {
            AGK_WARN("NPCController: NPC '{0}' has invalid interaction range: {1}",
                m_npcData.npcId, m_npcData.interactionRange);
        }

        if (m_npcData.aiUpdateInterval <= 0.0f) {
            AGK_WARN("NPCController: NPC '{0}' has invalid AI update interval: {1}",
                m_npcData.npcId, m_npcData.aiUpdateInterval);
        }
    }

    void NPCController::LogStateTransition(NPCState oldState, NPCState newState, const String& reason) const {
        AGK_INFO("NPCController: State transition for NPC '{0}' - {1} -> {2} (Reason: {3})",
            m_npcData.npcId, (int)oldState, (int)newState, reason);
    }

    // ==================================================================================
    // NPCUtils Implementation
    // ==================================================================================

    namespace NPCUtils {

        F32 CalculateDistance(const Vector3& pos1, const Vector3& pos2) {
            return pos1.DistanceTo(pos2);
        }

        bool IsPositionInRange(const Vector3& pos1, const Vector3& pos2, F32 range) {
            return CalculateDistance(pos1, pos2) <= range;
        }

        bool AreFactionsHostile(NPCFaction faction1, NPCFaction faction2) {
            // Define faction hostilities based on your lore
            if (faction1 == NPCFaction::Neutral || faction2 == NPCFaction::Neutral) {
                return false; // Neutrals are not hostile to anyone
            }

            // Example hostilities - adjust based on your game lore
            if ((faction1 == NPCFaction::Vaikuntha && faction2 == NPCFaction::YugaStriders) ||
                (faction1 == NPCFaction::YugaStriders && faction2 == NPCFaction::Vaikuntha)) {
                return true; // Corporate vs Rebels
            }

            // Ashvattha might have philosophical disagreements but not outright hostility
            return false;
        }

        F32 GetFactionRelationshipModifier(NPCFaction npcFaction, NPCFaction playerFaction) {
            if (npcFaction == playerFaction) {
                return 1.5f; // Same faction bonus
            }

            if (AreFactionsHostile(npcFaction, playerFaction)) {
                return 0.3f; // Hostile faction penalty
            }

            return 1.0f; // Neutral relationship
        }

        bool IsInteractiveState(NPCState state) {
            switch (state) {
            case NPCState::Idle:
            case NPCState::Working:
            case NPCState::Patrol:
                return true;

            case NPCState::Conversation:
            case NPCState::Sleeping:
            case NPCState::Hostile:
                return false;

            default:
                return false;
            }
        }

        bool IsMovementState(NPCState state) {
            switch (state) {
            case NPCState::Patrol:
            case NPCState::Following:
            case NPCState::Alert:
                return true;

            default:
                return false;
            }
        }

        std::vector<NPCState> GetValidTransitions(NPCState currentState) {
            std::vector<NPCState> validStates;

            switch (currentState) {
            case NPCState::Idle:
                validStates = { NPCState::Patrol, NPCState::Conversation, NPCState::Alert, NPCState::Working };
                break;

            case NPCState::Patrol:
                validStates = { NPCState::Idle, NPCState::Alert, NPCState::Conversation };
                break;

            case NPCState::Conversation:
                validStates = { NPCState::Idle, NPCState::Alert };
                break;

            case NPCState::Alert:
                validStates = { NPCState::Idle, NPCState::Hostile, NPCState::Patrol };
                break;

            case NPCState::Working:
                validStates = { NPCState::Idle, NPCState::Alert };
                break;

            case NPCState::Sleeping:
                validStates = { NPCState::Idle, NPCState::Alert };
                break;

            case NPCState::Following:
                validStates = { NPCState::Idle, NPCState::Alert, NPCState::Conversation };
                break;

            case NPCState::Hostile:
                validStates = { NPCState::Alert, NPCState::Idle };
                break;

            default:
                validStates = { NPCState::Idle };
                break;
            }

            return validStates;
        }

        String FormatPersonalityForAI(const NPCPersonality& personality) {
            std::stringstream ss;
            ss << "Personality: ";
            ss << "Aggressive=" << personality.aggressiveness << ", ";
            ss << "Curious=" << personality.curiosity << ", ";
            ss << "Trusting=" << personality.trustfulness << ", ";
            ss << "Helpful=" << personality.helpfulness << ", ";
            ss << "Intelligent=" << personality.intelligence << ", ";
            ss << "Loyal=" << personality.loyalty;

            return ss.str();
        }

        String FormatRelationshipForAI(const NPCRelationship& relationship) {
            std::stringstream ss;
            ss << "Relationship: ";
            ss << "Trust=" << relationship.trust << ", ";
            ss << "Respect=" << relationship.respect << ", ";
            ss << "Fear=" << relationship.fear << ", ";
            ss << "Affection=" << relationship.affection << ", ";
            ss << "Conversations=" << relationship.conversationCount;

            return ss.str();
        }

        String FormatKnowledgeForAI(const NPCKnowledge& knowledge) {
            std::stringstream ss;
            ss << "Knowledge: ";
            ss << "Topics=" << knowledge.knownTopics.size() << ", ";
            ss << "RecentEvents=" << knowledge.recentEvents.size();

            if (!knowledge.recentEvents.empty()) {
                ss << " (Latest: " << knowledge.recentEvents.back() << ")";
            }

            if (!knowledge.lastPlayerAction.empty()) {
                ss << ", LastPlayerAction=" << knowledge.lastPlayerAction;
            }

            return ss.str();
        }

    } // namespace NPCUtils

} // namespace Angaraka::AI