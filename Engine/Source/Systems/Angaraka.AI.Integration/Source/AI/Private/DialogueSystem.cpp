#include <Angaraka/AIBase.hpp>
#include "Angaraka/DialogueSystem.hpp"
#include "Angaraka/AIManager.hpp"
#include "Angaraka/NPCManager.hpp"
#include <sstream>

using namespace Angaraka::Core;

namespace Angaraka::AI {

    // ==================================================================================
    // Constructor and Destructor
    // ==================================================================================

    DialogueSystem::DialogueSystem(
        Reference<CachedResourceManager> resourceManager,
        Reference<AIManager> aiManager,
        Reference<NPCManager> npcManager)
        : m_resourceManager(resourceManager)
        , m_aiManager(aiManager)
        , m_npcManager(npcManager)
        , m_activeDialogue(nullptr)
        , m_isInitialized(false)
        , m_lastUpdate(std::chrono::steady_clock::now())
        , m_totalConversations(0)
        , m_totalConversationTime(0.0f)
        , m_totalExchanges(0)
    {
        AGK_INFO("DialogueSystem: Creating dialogue management system");
    }

    DialogueSystem::~DialogueSystem() {
        AGK_INFO("DialogueSystem: Destroying dialogue management system");
        Shutdown();
    }

    // ==================================================================================
    // Core Lifecycle
    // ==================================================================================

    bool DialogueSystem::Initialize(const DialogueSystemSettings& settings) {
        if (m_isInitialized) {
            AGK_WARN("DialogueSystem: Already initialized");
            return true;
        }

        AGK_INFO("DialogueSystem: Initializing with message speed: {0}, timeout: {1}s",
            settings.messageDisplaySpeed, settings.conversationTimeout);

        // Validate required systems
        if (!m_aiManager || !m_npcManager) {
            AGK_ERROR("DialogueSystem: Missing required system dependencies");
            return false;
        }

        // Store settings
        m_settings = settings;

        // Subscribe to events
        SubscribeToEvents();

        // Initialize caches
        m_choiceCache.reserve(100);
        m_relationshipCache.reserve(50);

        // Load conversation history if available
        LoadConversationHistory();

        m_isInitialized = true;
        AGK_INFO("DialogueSystem: Initialization complete");

        return true;
    }

    void DialogueSystem::Update(F32 deltaTime) {
        if (!m_isInitialized) {
            return;
        }

        // Update active dialogue state
        if (m_activeDialogue) {
            ProcessConversationState(deltaTime);
        }

        // Periodic cleanup
        static F32 cleanupTimer = 0.0f;
        cleanupTimer += deltaTime;
        if (cleanupTimer >= 30.0f) { // Every 30 seconds
            cleanupTimer = 0.0f;
            PurgeOldCaches();
            CleanupOldConversations();
        }

        m_lastUpdate = std::chrono::steady_clock::now();
    }

    void DialogueSystem::Shutdown() {
        if (!m_isInitialized) {
            return;
        }

        AGK_INFO("DialogueSystem: Shutting down");

        // End any active conversation
        if (m_activeDialogue) {
            EndConversation(DialogueEndReason::Interrupted);
        }

        // Save conversation history
        SaveConversationHistory();

        // Unsubscribe from events
        UnsubscribeFromEvents();

        // Clear caches and data
        m_conversationHistory.clear();
        m_choiceCache.clear();
        m_relationshipCache.clear();

        // Clear callbacks
        m_startCallback = nullptr;
        m_endCallback = nullptr;
        m_messageCallback = nullptr;
        m_choiceCallback = nullptr;

        m_isInitialized = false;
        AGK_INFO("DialogueSystem: Shutdown complete");
    }

    // ==================================================================================
    // Conversation Management
    // ==================================================================================

    bool DialogueSystem::StartConversation(const String& npcId, const String& initialTopic) {
        if (!m_isInitialized) {
            AGK_ERROR("DialogueSystem: Cannot start conversation - system not initialized");
            return false;
        }

        if (m_activeDialogue) {
            AGK_WARN("DialogueSystem: Ending existing conversation before starting new one");
            EndConversation(DialogueEndReason::Interrupted);
        }

        // Validate NPC
        if (!ValidateNPCForConversation(npcId)) {
            AGK_ERROR("DialogueSystem: Cannot start conversation with NPC '{0}' - validation failed", npcId);
            return false;
        }

        NPCController* npc = m_npcManager->GetNPC(npcId);
        if (!npc) {
            AGK_ERROR("DialogueSystem: NPC '{0}' not found", npcId);
            return false;
        }

        const NPCComponent& npcData = npc->GetNPCData();

        AGK_INFO("DialogueSystem: Starting conversation with NPC '{0}' (Faction: {1})",
            npcId, npcData.GetFactionString());

        // Create active dialogue
        m_activeDialogue = std::make_unique<ActiveDialogue>();
        m_activeDialogue->npcId = npcId;
        m_activeDialogue->npcName = npcData.displayName;
        m_activeDialogue->npcFaction = npcData.faction;
        m_activeDialogue->currentState = DialogueState::Starting;
        m_activeDialogue->startTime = std::chrono::steady_clock::now();
        m_activeDialogue->lastExchangeTime = m_activeDialogue->startTime;
        m_activeDialogue->timeoutDuration = m_settings.conversationTimeout;
        m_activeDialogue->currentTopic = initialTopic.empty() ? "greeting" : initialTopic;
        m_activeDialogue->conversationContext = BuildConversationContext();

        // Store initial relationship values
        m_activeDialogue->initialTrust = npcData.relationship.trust;
        m_activeDialogue->initialRespect = npcData.relationship.respect;

        // Generate initial greeting from NPC
        String greetingContext = "greeting " + m_activeDialogue->currentTopic;

        // Execute start callback
        if (m_startCallback) {
            m_startCallback(npcId);
        }

        // (npcId, npcData.displayName, npcData.faction, npcData.position, "");
        // Publish start event
        DialogueStartedEvent startEvent;
        startEvent.npcId = npcId;
        startEvent.npcName = npcData.displayName;
        startEvent.npcFaction = npcData.faction;
        startEvent.npcPosition = npcData.position;
        startEvent.initialMessage = "";
        
        Angaraka::Events::EventManager::Get().Broadcast(startEvent);

        // Note: Actual event publishing would depend on your EventManager API
        AGK_INFO("DialogueSystem: Publishing dialogue started event for NPC '{0}'", npcId);

        // Update NPC state
        npc->ChangeState(NPCState::Conversation, "Started dialogue with player");

        // Update statistics
        m_totalConversations++;
        TransitionToState(DialogueState::WaitingForNPC);
        RequestAIDialogue("", greetingContext);
        return true;
    }

    void DialogueSystem::EndConversation(DialogueEndReason reason) {
        if (!m_activeDialogue) {
            return;
        }

        String npcId = m_activeDialogue->npcId;

        auto endTime = std::chrono::steady_clock::now();
        F32 conversationDuration = std::chrono::duration<F32>(endTime - m_activeDialogue->startTime).count();

        AGK_INFO("DialogueSystem: Ending conversation with NPC '{0}' after {1:.1f}s ({2} exchanges)",
            npcId, conversationDuration, m_activeDialogue->exchanges.size());

        // Calculate relationship change
        F32 relationshipChange = m_activeDialogue->relationshipChangeThisConversation;

        // Update conversation metrics
        m_totalConversationTime += conversationDuration;
        m_totalExchanges += static_cast<U32>(m_activeDialogue->exchanges.size());

        // Save conversation to history
        if (!m_activeDialogue->exchanges.empty()) {
            m_conversationHistory[npcId] = m_activeDialogue->exchanges;
        }

        // Execute end callback
        if (m_endCallback) {
            m_endCallback(npcId, reason);
        }

        // Publish end event
        DialogueEndedEvent endEvent;
        endEvent.npcId = npcId;
        endEvent.reason = reason == DialogueEndReason::PlayerChoice ? "player_choice" :
            reason == DialogueEndReason::NPCDecision ? "npc_decision" :
            reason == DialogueEndReason::Timeout ? "timeout" :
            reason == DialogueEndReason::Interrupted ? "interrupted" : "error";
        endEvent.conversationDuration = conversationDuration;
        endEvent.exchangeCount = static_cast<U32>(m_activeDialogue->exchanges.size());
        endEvent.relationshipChange = relationshipChange;
        Angaraka::Events::EventManager::Get().Broadcast(endEvent);

        // Note: Actual event publishing would depend on your EventManager API
        AGK_INFO("DialogueSystem: Publishing dialogue ended event for NPC '{0}'", npcId);

        // Return NPC to appropriate state
        NPCController* npc = m_npcManager->GetNPC(npcId);
        if (npc) {
            npc->ChangeState(NPCState::Idle, "Conversation ended");
        }

        // Clear active dialogue
        m_activeDialogue.reset();
    }

    void DialogueSystem::PauseConversation() {
        if (!m_activeDialogue || m_activeDialogue->isPaused) {
            return;
        }

        m_activeDialogue->isPaused = true;
        AGK_INFO("DialogueSystem: Paused conversation with NPC '{0}'", m_activeDialogue->npcId);
    }

    void DialogueSystem::ResumeConversation() {
        if (!m_activeDialogue || !m_activeDialogue->isPaused) {
            return;
        }

        m_activeDialogue->isPaused = false;
        m_activeDialogue->lastExchangeTime = std::chrono::steady_clock::now();
        AGK_INFO("DialogueSystem: Resumed conversation with NPC '{0}'", m_activeDialogue->npcId);
    }

    // ==================================================================================
    // Player Interaction
    // ==================================================================================

    void DialogueSystem::SelectChoice(U32 choiceIndex) {
        if (!m_activeDialogue || !ValidateChoiceIndex(choiceIndex)) {
            AGK_WARN("DialogueSystem: Invalid choice selection - index {0}", choiceIndex);
            return;
        }

        if (m_activeDialogue->currentState != DialogueState::WaitingForChoice) {
            AGK_WARN("DialogueSystem: Cannot select choice in current state");
            return;
        }

        const DialogueChoice& choice = m_activeDialogue->currentChoices[choiceIndex];

        DialogueChoiceSelectedEvent choiceEvent;
        choiceEvent.npcId = m_activeDialogue->npcId;
        choiceEvent.playerChoice = choice.choiceText;
        choiceEvent.choiceIndex = choiceIndex;
        choiceEvent.choiceContext = m_activeDialogue->conversationContext;

        if (!choice.isAvailable) {
            AGK_WARN("DialogueSystem: Selected choice is not available: {0}", choice.unavailableReason);
            return;
        }

        AGK_INFO("DialogueSystem: Player selected choice {0}: '{1}'", choiceIndex, choice.choiceText);

        // Process the choice
        ProcessPlayerChoice(choice.choiceText, choice.choiceType);

        // Execute choice callback
        if (m_choiceCallback) {
            m_choiceCallback(m_activeDialogue->npcId, choiceIndex);
        }

        // Publish choice event
        Angaraka::Events::EventManager::Get().Broadcast(choiceEvent);

        // Note: Actual event publishing would depend on your EventManager API
        AGK_INFO("DialogueSystem: Publishing choice selected event");

    }

    void DialogueSystem::SelectChoice(const String& choiceText) {
        if (!m_activeDialogue) {
            return;
        }

        // Find choice by text
        for (U32 i = 0; i < m_activeDialogue->currentChoices.size(); ++i) {
            if (m_activeDialogue->currentChoices[i].choiceText == choiceText) {
                SelectChoice(i);
                return;
            }
        }

        AGK_WARN("DialogueSystem: Choice text not found: '{0}'", choiceText);
    }

    std::vector<DialogueChoice> DialogueSystem::GetCurrentChoices() const {
        if (!m_activeDialogue) {
            return {};
        }

        return m_activeDialogue->currentChoices;
    }

    bool DialogueSystem::CanMakeChoice() const {
        return m_activeDialogue &&
            m_activeDialogue->currentState == DialogueState::WaitingForChoice &&
            !m_activeDialogue->currentChoices.empty();
    }

    // ==================================================================================
    // Conversation State
    // ==================================================================================

    bool DialogueSystem::IsInConversation() const {
        return m_activeDialogue && m_activeDialogue->currentState != DialogueState::Inactive;
    }

    const ActiveDialogue* DialogueSystem::GetActiveDialogue() const {
        return m_activeDialogue.get();
    }

    // ==================================================================================
    // AI Integration
    // ==================================================================================

    void DialogueSystem::ProcessPlayerChoice(const String& choice, const String& choiceType) {
        if (!m_activeDialogue) {
            return;
        }

        // Add player message to exchange history
        AddExchange("player", choice, "determined");

        // Update relationship based on choice
        NPCController* npc = m_npcManager->GetNPC(m_activeDialogue->npcId);
        if (npc) {
            F32 relationshipImpact = CalculateRelationshipImpact(choiceType, m_activeDialogue->npcId);
            TrackRelationshipChange(m_activeDialogue->npcId, relationshipImpact);

            // Update the NPC's relationship
            if (relationshipImpact != 0.0f) {
                F32 trustChange = relationshipImpact * 0.6f;
                F32 respectChange = relationshipImpact * 0.4f;
                npc->UpdateRelationship(trustChange, respectChange);

                AGK_DEBUG("DialogueSystem: Relationship impact {0:.2f} for choice type '{1}'",
                    relationshipImpact, choiceType);
            }
        }

        // Store pending choice context
        m_activeDialogue->pendingPlayerChoice = choice;
        m_activeDialogue->pendingChoiceContext = choiceType;

        // Request AI response
        String conversationContext = BuildConversationContext();
        TransitionToState(DialogueState::WaitingForNPC);

        RequestAIDialogue(choice, conversationContext);
    }

    void DialogueSystem::HandleAIResponse(const String& npcId, const String& response, const String& emotionalTone) {
        if (!m_activeDialogue || m_activeDialogue->npcId != npcId) {
            AGK_WARN("DialogueSystem: Received AI response for wrong NPC or no active dialogue");
            return;
        }

        if (m_activeDialogue->currentState != DialogueState::WaitingForNPC) {
            AGK_WARN("DialogueSystem: Received AI response in wrong state");
            return;
        }

        AGK_INFO("DialogueSystem: Received AI response with tone '{0}': '{1}'", emotionalTone, response);

        // Add NPC response to exchange history
        AddExchange(npcId, response, emotionalTone);

        // Update AI performance metrics
        m_activeDialogue->aiRequestCount++;
        auto now = std::chrono::steady_clock::now();
        F32 responseTime = std::chrono::duration<F32, std::milli>(now - m_activeDialogue->lastExchangeTime).count();
        m_activeDialogue->totalAIResponseTime += responseTime;
        m_activeDialogue->averageResponseTime = m_activeDialogue->totalAIResponseTime / m_activeDialogue->aiRequestCount;

        // Execute message callback
        if (m_messageCallback) {
            DialogueTone tone = ParseEmotionalTone(emotionalTone);
            m_messageCallback(npcId, response, tone);
        }

        // Publish message event
        DialogueMessageEvent messageEvent;
        messageEvent.npcId = npcId;
        messageEvent.message = response;
        messageEvent.emotionalTone = emotionalTone;
        messageEvent.deliverySpeed = m_settings.messageDisplaySpeed;
        messageEvent.isPlayerMessage = false; // NPC response
        Angaraka::Events::EventManager::Get().Broadcast(messageEvent);
        // Note: Actual event publishing would depend on your EventManager API
        AGK_INFO("DialogueSystem: Publishing dialogue message event");

        // Generate player choices for response
        m_activeDialogue->currentChoices = GeneratePlayerChoices(response, m_activeDialogue->conversationContext);

        // Check if conversation should end
        if (m_activeDialogue->exchanges.size() >= m_settings.maxExchangesPerConversation) {
            AGK_INFO("DialogueSystem: Conversation reached maximum exchanges, ending");
            EndConversation(DialogueEndReason::NPCDecision);
            return;
        }

        // Check for conversation ending keywords in NPC response
        if (response.find("goodbye") != String::npos ||
            response.find("farewell") != String::npos ||
            response.find("must go") != String::npos) {
            AGK_INFO("DialogueSystem: NPC indicated conversation end");
            EndConversation(DialogueEndReason::NPCDecision);
            return;
        }

        TransitionToState(DialogueState::WaitingForChoice);
    }

    void DialogueSystem::RequestAIDialogue(const String& playerMessage, const String& context) {
        if (!m_aiManager || !m_activeDialogue) {
            return;
        }

        AGK_DEBUG("DialogueSystem: Requesting AI dialogue for NPC '{0}'", m_activeDialogue->npcId);

        m_activeDialogue->waitingForAIResponse = true;
        m_activeDialogue->lastExchangeTime = std::chrono::steady_clock::now();

        // Get NPC for faction information
        NPCController* npc = m_npcManager->GetNPC(m_activeDialogue->npcId);
        if (!npc) {
            HandleConversationError("NPC not found for AI request");
            return;
        }

        const NPCComponent& npcData = npc->GetNPCData();

        // Prepare dialogue request
        DialogueRequest request;
        request.factionId = npcData.GetFactionString();
        request.npcId = m_activeDialogue->npcId;
        request.playerMessage = playerMessage;
        request.conversationContext = context;

        // Add emotional state from relationship
        request.emotionalState["trust"] = npcData.relationship.trust;
        request.emotionalState["respect"] = npcData.relationship.respect;
        request.emotionalState["fear"] = npcData.relationship.fear;

        // Add recent events from conversation
        for (const auto& exchange : m_activeDialogue->exchanges) {
            if (exchange.speaker == "player") {
                request.recentEvents.push_back("Player: " + exchange.message);
            }
        }

        request.urgency = 0.5f; // Normal conversation urgency

        // Send request to AI system
        auto response = m_aiManager->GenerateDialogueSync(request);

        if (response.success) {
            HandleAIResponse(m_activeDialogue->npcId, response.response, response.emotionalTone);
        }
        else {
            AGK_ERROR("DialogueSystem: AI dialogue request failed for NPC '{0}'", m_activeDialogue->npcId);
            HandleConversationError("AI dialogue generation failed");
        }
    }

    // ==================================================================================
    // Conversation History and Analysis
    // ==================================================================================

    std::vector<DialogueExchange> DialogueSystem::GetConversationHistory(const String& npcId) const {
        auto it = m_conversationHistory.find(npcId);
        return it != m_conversationHistory.end() ? it->second : std::vector<DialogueExchange>{};
    }

    F32 DialogueSystem::GetRelationshipChange(const String& npcId) const {
        auto it = m_relationshipCache.find(npcId);
        return it != m_relationshipCache.end() ? it->second : 0.0f;
    }

    String DialogueSystem::GetConversationSummary(const String& npcId) const {
        auto history = GetConversationHistory(npcId);
        if (history.empty()) {
            return "No conversation history";
        }

        std::stringstream summary;
        summary << "Conversation with " << npcId << " (" << history.size() << " exchanges):\n";

        // Get last few exchanges
        size_t startIndex = history.size() > 3 ? history.size() - 3 : 0;
        for (size_t i = startIndex; i < history.size(); ++i) {
            const auto& exchange = history[i];
            String speaker = exchange.speaker == "player" ? "Player" : "NPC";
            summary << speaker << ": " << exchange.message << "\n";
        }

        F32 relationshipChange = GetRelationshipChange(npcId);
        if (relationshipChange != 0.0f) {
            summary << "Relationship change: " << (relationshipChange > 0 ? "+" : "") << relationshipChange;
        }

        return summary.str();
    }

    bool DialogueSystem::SaveConversationHistory() {
        // Note: Actual file saving would depend on your file system integration
        // This is a placeholder implementation

        AGK_INFO("DialogueSystem: Saving conversation history for {0} NPCs", m_conversationHistory.size());

        // Example: Save to JSON or binary format
        // The conversation history could be serialized for persistence

        return true;
    }

    bool DialogueSystem::LoadConversationHistory() {
        // Note: Actual file loading would depend on your file system integration
        // This is a placeholder implementation

        AGK_INFO("DialogueSystem: Loading conversation history");

        // Example: Load from saved file
        // Deserialize conversation data and populate m_conversationHistory

        return true;
    }

    // ==================================================================================
    // Choice Generation and Validation
    // ==================================================================================

    std::vector<DialogueChoice> DialogueSystem::GeneratePlayerChoices(const String& npcResponse, const String& context) {
        if (!m_activeDialogue) {
            return {};
        }

        // Check cache first
        String cacheKey = m_activeDialogue->npcId + "_" + context;
        auto cachedIt = m_choiceCache.find(cacheKey);
        if (cachedIt != m_choiceCache.end()) {
            AGK_DEBUG("DialogueSystem: Using cached choices for context '{0}'", context);
            return cachedIt->second;
        }

        // Generate contextual choices
        std::vector<DialogueChoice> choices = CreateContextualChoices(npcResponse, m_activeDialogue->npcId);

        // Add default choices if needed
        if (choices.size() < 2) {
            auto defaultChoices = CreateDefaultChoices(context);
            choices.insert(choices.end(), defaultChoices.begin(), defaultChoices.end());
        }

        // Limit to max choices
        if (choices.size() > m_settings.maxPlayerChoices) {
            choices.resize(m_settings.maxPlayerChoices);
        }

        // Validate and populate metadata for each choice
        for (auto& choice : choices) {
            ValidateChoiceAvailability(choice, m_activeDialogue->npcId);
            PopulateChoiceMetadata(choice, m_activeDialogue->npcId);
        }

        // Cache the choices
        m_choiceCache[cacheKey] = choices;

        AGK_DEBUG("DialogueSystem: Generated {0} player choices", choices.size());
        return choices;
    }

    void DialogueSystem::ValidateChoiceAvailability(DialogueChoice& choice, const String& npcId) {
        if (!choice.requiresCheck) {
            choice.isAvailable = true;
            return;
        }

        // Get NPC for relationship checks
        NPCController* npc = m_npcManager->GetNPC(npcId);
        if (!npc) {
            choice.isAvailable = false;
            choice.unavailableReason = "NPC not found";
            return;
        }

        const NPCComponent& npcData = npc->GetNPCData();

        // Simple skill check simulation
        F32 playerSkill = 0.5f; // This would come from player stats in a real implementation
        F32 relationshipBonus = (npcData.relationship.trust + npcData.relationship.respect) * 0.1f;
        F32 totalSkill = playerSkill + relationshipBonus;

        if (totalSkill >= choice.checkDifficulty) {
            choice.isAvailable = true;
        }
        else {
            choice.isAvailable = false;
            choice.unavailableReason = "Requires " + choice.checkType + " skill";
        }
    }

    F32 DialogueSystem::PredictChoiceOutcome(const DialogueChoice& choice, const String& npcId) {
        // Simple prediction based on choice type and NPC faction
        F32 baseImpact = choice.relationshipImpact;

        NPCController* npc = m_npcManager->GetNPC(npcId);
        if (!npc) {
            return baseImpact;
        }

        const NPCComponent& npcData = npc->GetNPCData();

        // Faction-specific modifiers
        if (choice.choiceType == "diplomatic" && npcData.faction == NPCFaction::Ashvattha) {
            baseImpact *= 1.2f; // Ashvattha appreciates diplomacy
        }
        else if (choice.choiceType == "aggressive" && npcData.faction == NPCFaction::YugaStriders) {
            baseImpact *= 0.8f; // Yuga Striders might understand aggression better
        }
        else if (choice.choiceType == "logical" && npcData.faction == NPCFaction::Vaikuntha) {
            baseImpact *= 1.3f; // Vaikuntha values logic
        }

        return Clamp(baseImpact, -1.0f, 1.0f);
    }

    // ==================================================================================
    // Settings and Configuration
    // ==================================================================================

    void DialogueSystem::UpdateSettings(const DialogueSystemSettings& newSettings) {
        AGK_INFO("DialogueSystem: Updating settings - Message speed: {0}, Timeout: {1}s",
            newSettings.messageDisplaySpeed, newSettings.conversationTimeout);

        m_settings = newSettings;

        // Apply new timeout to active dialogue
        if (m_activeDialogue) {
            m_activeDialogue->timeoutDuration = newSettings.conversationTimeout;
        }
    }

    // ==================================================================================
    // UI Integration Helpers
    // ==================================================================================

    String DialogueSystem::GetFormattedMessage(const DialogueExchange& exchange) const {
        std::stringstream formatted;

        String speakerName = exchange.speaker == "player" ? "Player" :
            (m_activeDialogue ? m_activeDialogue->npcName : "NPC");

        formatted << speakerName << ": " << exchange.message;

        if (m_settings.enableDebugInfo) {
            formatted << " [" << exchange.emotionalTone << "]";
        }

        return formatted.str();
    }

    String DialogueSystem::GetRelationshipStatusText(const String& npcId) const {
        NPCController* npc = m_npcManager->GetNPC(npcId);
        if (!npc) {
            return "Unknown";
        }

        const NPCRelationship& rel = npc->GetRelationship();

        if (rel.trust > 0.8f && rel.respect > 0.8f) {
            return "Trusted Ally";
        }
        else if (rel.trust > 0.6f && rel.respect > 0.6f) {
            return "Good Friend";
        }
        else if (rel.trust > 0.4f && rel.respect > 0.4f) {
            return "Friendly";
        }
        else if (rel.fear > 0.6f) {
            return "Fearful";
        }
        else if (rel.trust < 0.3f && rel.respect < 0.3f) {
            return "Hostile";
        }
        else {
            return "Neutral";
        }
    }

    String DialogueSystem::GetChoicePreviewText(const DialogueChoice& choice) const {
        std::stringstream preview;
        preview << choice.choiceText;

        if (!choice.isAvailable) {
            preview << " [" << choice.unavailableReason << "]";
        }
        else if (choice.requiresCheck) {
            preview << " [" << choice.checkType << " check]";
        }

        if (m_settings.enableRelationshipFeedback && choice.relationshipImpact != 0.0f) {
            preview << " (" << (choice.relationshipImpact > 0 ? "+" : "") << choice.relationshipImpact << ")";
        }

        return preview.str();
    }

    // ==================================================================================
    // Debug and Diagnostics
    // ==================================================================================

    String DialogueSystem::GetDebugInfo() const {
        std::stringstream debug;

        debug << "=== DialogueSystem Debug Info ===\n";
        debug << "Initialized: " << (m_isInitialized ? "Yes" : "No") << "\n";
        debug << "Active Dialogue: " << (m_activeDialogue ? m_activeDialogue->npcId : "None") << "\n";

        if (m_activeDialogue) {
            debug << "State: " << static_cast<int>(m_activeDialogue->currentState) << "\n";
            debug << "Exchanges: " << m_activeDialogue->exchanges.size() << "\n";
            debug << "Choices: " << m_activeDialogue->currentChoices.size() << "\n";
            debug << "AI Requests: " << m_activeDialogue->aiRequestCount << "\n";
            debug << "Avg Response Time: " << m_activeDialogue->averageResponseTime << "ms\n";
        }

        debug << "Total Conversations: " << m_totalConversations << "\n";
        debug << "Cached Choices: " << m_choiceCache.size() << "\n";
        debug << "Conversation History: " << m_conversationHistory.size() << " NPCs\n";

        if (m_totalConversations > 0) {
            F32 avgDuration = m_totalConversationTime / m_totalConversations;
            F32 avgExchanges = static_cast<F32>(m_totalExchanges) / m_totalConversations;
            debug << "Avg Conversation Duration: " << avgDuration << "s\n";
            debug << "Avg Exchanges per Conversation: " << avgExchanges << "\n";
        }

        return debug.str();
    }

    void DialogueSystem::LogConversationMetrics() const {
        if (m_totalConversations == 0) {
            AGK_INFO("DialogueSystem: No conversation metrics available");
            return;
        }

        F32 avgDuration = m_totalConversationTime / m_totalConversations;
        F32 avgExchanges = static_cast<F32>(m_totalExchanges) / m_totalConversations;

        AGK_INFO("DialogueSystem Metrics: {0} conversations, {1:.1f}s avg duration, {2:.1f} avg exchanges",
            m_totalConversations, avgDuration, avgExchanges);
    }

    F32 DialogueSystem::GetAverageAIResponseTime() const {
        return m_activeDialogue ? m_activeDialogue->averageResponseTime : 0.0f;
    }

    // ==================================================================================
    // Private Helper Methods
    // ==================================================================================

    void DialogueSystem::SubscribeToEvents() {
        // Note: Actual event subscription would depend on your EventManager API
        // Example subscriptions:
        // Angaraka::Events::EventManager::Get().Subscribe<NPCInteractionEvent>([this](const NPCDialogueResponseEvent& e) { OnNPCDialogueResponse(e); });

        AGK_INFO("DialogueSystem: Subscribed to dialogue events");
    }

    void DialogueSystem::UnsubscribeFromEvents() {
        // Note: Actual event unsubscription would depend on your EventManager API
        AGK_INFO("DialogueSystem: Unsubscribed from dialogue events");
    }

    void DialogueSystem::OnNPCDialogueResponse(const NPCDialogueResponseEvent& response) {
        HandleAIResponse(response.npcId, response.npcResponse, response.emotionalTone);
    }

    void DialogueSystem::OnNPCStateChange(const NPCStateChangeEvent& stateChange) {
        // If NPC leaves conversation state unexpectedly, end dialogue
        if (m_activeDialogue && m_activeDialogue->npcId == stateChange.npcId &&
            stateChange.newState != NPCState::Conversation &&
            m_activeDialogue->currentState != DialogueState::Ending) {

            AGK_INFO("DialogueSystem: NPC '{0}' left conversation state, ending dialogue", stateChange.npcId);
            EndConversation(DialogueEndReason::Interrupted);
        }
    }

    void DialogueSystem::OnPlayerInteraction(const NPCInteractionEvent& interaction) {
        // Auto-start dialogue for dialogue interactions
        if (interaction.interactionType == InteractionType::Dialogue && !IsInConversation()) {
            StartConversation(interaction.npcId);
        }
    }

    void DialogueSystem::ProcessConversationState(F32 deltaTime) {
        if (!m_activeDialogue || m_activeDialogue->isPaused) {
            return;
        }

        // Check for timeout
        auto now = std::chrono::steady_clock::now();
        F32 timeSinceLastExchange = std::chrono::duration<F32>(now - m_activeDialogue->lastExchangeTime).count();

        if (timeSinceLastExchange > m_activeDialogue->timeoutDuration) {
            AGK_INFO("DialogueSystem: Conversation timeout after {0:.1f}s", timeSinceLastExchange);
            EndConversation(DialogueEndReason::Timeout);
            return;
        }

        // State-specific processing
        switch (m_activeDialogue->currentState) {
        case DialogueState::Starting:
            // Waiting for initial AI response
            if (timeSinceLastExchange > m_settings.aiResponseTimeout) {
                HandleConversationError("AI response timeout during start");
            }
            break;

        case DialogueState::WaitingForNPC:
            // Waiting for AI response to player choice
            if (timeSinceLastExchange > m_settings.aiResponseTimeout) {
                HandleConversationError("AI response timeout");
            }
            break;

        case DialogueState::WaitingForChoice:
            // Player taking time to choose - this is normal
            break;

        case DialogueState::DisplayingMessage:
            {
                // Message being displayed to player
                F32 displayTime = CalculateTypingDuration(m_activeDialogue->exchanges.back().message);
                if (timeSinceLastExchange > displayTime + 1.0f) {
                    TransitionToState(DialogueState::WaitingForChoice);
                }
            }
            break;

        case DialogueState::Processing:
            // Processing player choice
            if (timeSinceLastExchange > 2.0f) {
                TransitionToState(DialogueState::WaitingForNPC);
            }
            break;

        default:
            break;
        }
    }

    void DialogueSystem::TransitionToState(DialogueState newState) {
        if (!m_activeDialogue) {
            return;
        }

        DialogueState oldState = m_activeDialogue->currentState;
        m_activeDialogue->currentState = newState;

        AGK_DEBUG("DialogueSystem: State transition {0} -> {1}", static_cast<int>(oldState), static_cast<int>(newState));

        // State entry actions
        switch (newState) {
        case DialogueState::WaitingForChoice:
            // Ensure choices are available
            if (m_activeDialogue->currentChoices.empty()) {
                m_activeDialogue->currentChoices = CreateDefaultChoices("default");
            }
            break;

        case DialogueState::Ending:
            // Prepare for conversation end
            m_activeDialogue->waitingForAIResponse = false;
            break;

        default:
            break;
        }
    }

    void DialogueSystem::HandleStateTimeout() {
        if (!m_activeDialogue) {
            return;
        }

        AGK_WARN("DialogueSystem: State timeout in state {0}", static_cast<int>(m_activeDialogue->currentState));

        switch (m_activeDialogue->currentState) {
        case DialogueState::WaitingForNPC:
            // AI didn't respond, create a generic response
            HandleAIResponse(m_activeDialogue->npcId, "I need to think about that.", "thoughtful");
            break;

        case DialogueState::WaitingForChoice:
            // Player didn't choose, auto-select a safe option
            if (!m_activeDialogue->currentChoices.empty()) {
                SelectChoice(0);
            }
            break;

        default:
            EndConversation(DialogueEndReason::Timeout);
            break;
        }
    }

    String DialogueSystem::BuildConversationContext() const {
        if (!m_activeDialogue) {
            return "";
        }

        std::stringstream context;
        context << "NPC: " << m_activeDialogue->npcName;
        context << ", Faction: " << static_cast<int>(m_activeDialogue->npcFaction);
        context << ", Topic: " << m_activeDialogue->currentTopic;

        // Add recent exchange context
        if (!m_activeDialogue->exchanges.empty()) {
            const auto& lastExchange = m_activeDialogue->exchanges.back();
            context << ", LastSpeaker: " << lastExchange.speaker;
            context << ", LastTone: " << lastExchange.emotionalTone;
        }

        // Add relationship context
        NPCController* npc = m_npcManager->GetNPC(m_activeDialogue->npcId);
        if (npc) {
            const NPCRelationship& rel = npc->GetRelationship();
            context << ", Trust: " << rel.trust;
            context << ", Respect: " << rel.respect;
        }

        return context.str();
    }

    String DialogueSystem::FormatConversationHistory() const {
        if (!m_activeDialogue || m_activeDialogue->exchanges.empty()) {
            return "";
        }

        std::stringstream history;

        // Include last few exchanges for context
        size_t startIndex = m_activeDialogue->exchanges.size() > 4 ? m_activeDialogue->exchanges.size() - 4 : 0;

        for (size_t i = startIndex; i < m_activeDialogue->exchanges.size(); ++i) {
            const auto& exchange = m_activeDialogue->exchanges[i];
            String speaker = exchange.speaker == "player" ? "Player" : "NPC";
            history << speaker << ": " << exchange.message << " ";
        }

        return history.str();
    }

    std::vector<DialogueChoice> DialogueSystem::CreateDefaultChoices(const String& context) {
        std::vector<DialogueChoice> choices;

        // Generic choices that work in most contexts
        DialogueChoice continueChoice;
        continueChoice.choiceText = "Continue the conversation";
        continueChoice.choiceType = "neutral";
        continueChoice.relationshipImpact = 0.0f;
        continueChoice.isAvailable = true;
        choices.push_back(continueChoice);

        DialogueChoice askQuestion;
        askQuestion.choiceText = "Ask a question";
        askQuestion.choiceType = "questioning";
        askQuestion.relationshipImpact = 0.05f;
        askQuestion.isAvailable = true;
        choices.push_back(askQuestion);

        DialogueChoice endChoice;
        endChoice.choiceText = "End the conversation";
        endChoice.choiceType = "polite_exit";
        endChoice.relationshipImpact = 0.0f;
        endChoice.isAvailable = true;
        choices.push_back(endChoice);

        return choices;
    }

    std::vector<DialogueChoice> DialogueSystem::CreateContextualChoices(const String& npcResponse, const String& npcId) {
        std::vector<DialogueChoice> choices;

        NPCController* npc = m_npcManager->GetNPC(npcId);
        if (!npc) {
            return choices;
        }

        const NPCComponent& npcData = npc->GetNPCData();

        // Faction-specific choices
        switch (npcData.faction) {
        case NPCFaction::Ashvattha:
        {
            DialogueChoice philosophical;
            philosophical.choiceText = "Tell me more about your philosophical perspective";
            philosophical.choiceType = "philosophical";
            philosophical.relationshipImpact = 0.1f;
            choices.push_back(philosophical);

            DialogueChoice respectful;
            respectful.choiceText = "I respect your wisdom";
            respectful.choiceType = "respectful";
            respectful.relationshipImpact = 0.15f;
            choices.push_back(respectful);
        }
        break;

        case NPCFaction::Vaikuntha:
        {
            DialogueChoice logical;
            logical.choiceText = "That's a logical approach";
            logical.choiceType = "logical";
            logical.relationshipImpact = 0.1f;
            choices.push_back(logical);

            DialogueChoice efficient;
            efficient.choiceText = "How can we optimize this process?";
            efficient.choiceType = "efficiency_focused";
            efficient.relationshipImpact = 0.12f;
            choices.push_back(efficient);
        }
        break;

        case NPCFaction::YugaStriders:
        {
            DialogueChoice supportive;
            supportive.choiceText = "I support your cause";
            supportive.choiceType = "supportive";
            supportive.relationshipImpact = 0.15f;
            choices.push_back(supportive);

            DialogueChoice rebellious;
            rebellious.choiceText = "The system needs to change";
            rebellious.choiceType = "rebellious";
            rebellious.relationshipImpact = 0.1f;
            choices.push_back(rebellious);
        }
        break;

        default:
            // Neutral faction gets generic choices
            break;
        }

        // Response-based contextual choices
        if (npcResponse.find("question") != String::npos || npcResponse.find("ask") != String::npos) {
            DialogueChoice answer;
            answer.choiceText = "Let me answer that";
            answer.choiceType = "helpful";
            answer.relationshipImpact = 0.05f;
            choices.push_back(answer);
        }

        if (npcResponse.find("disagree") != String::npos || npcResponse.find("wrong") != String::npos) {
            DialogueChoice diplomatic;
            diplomatic.choiceText = "Perhaps we can find common ground";
            diplomatic.choiceType = "diplomatic";
            diplomatic.relationshipImpact = 0.08f;
            choices.push_back(diplomatic);

            DialogueChoice challenge;
            challenge.choiceText = "I disagree with your assessment";
            challenge.choiceType = "challenging";
            challenge.relationshipImpact = -0.05f;
            choices.push_back(challenge);
        }

        return choices;
    }

    void DialogueSystem::PopulateChoiceMetadata(DialogueChoice& choice, const String& npcId) {
        // Predict relationship impact
        choice.relationshipImpact = PredictChoiceOutcome(choice, npcId);

        // Set expected response type
        if (choice.choiceType == "aggressive" || choice.choiceType == "challenging") {
            choice.expectedResponse = "defensive";
        }
        else if (choice.choiceType == "diplomatic" || choice.choiceType == "respectful") {
            choice.expectedResponse = "positive";
        }
        else if (choice.choiceType == "questioning") {
            choice.expectedResponse = "informative";
        }
        else {
            choice.expectedResponse = "neutral";
        }

        // Set skill check requirements for complex choices
        if (choice.choiceType == "persuasion") {
            choice.requiresCheck = true;
            choice.checkType = "persuasion";
            choice.checkDifficulty = 0.6f;
        }
        else if (choice.choiceType == "intimidation") {
            choice.requiresCheck = true;
            choice.checkType = "intimidation";
            choice.checkDifficulty = 0.7f;
        }
        else if (choice.choiceType == "philosophical") {
            choice.requiresCheck = true;
            choice.checkType = "knowledge";
            choice.checkDifficulty = 0.5f;
        }
    }

    DialogueTone DialogueSystem::ParseEmotionalTone(const String& toneString) const {
        if (toneString == "friendly" || toneString == "warm") return DialogueTone::Friendly;
        if (toneString == "hostile" || toneString == "angry") return DialogueTone::Hostile;
        if (toneString == "respectful" || toneString == "formal") return DialogueTone::Respectful;
        if (toneString == "dismissive" || toneString == "condescending") return DialogueTone::Dismissive;
        if (toneString == "philosophical" || toneString == "contemplative") return DialogueTone::Philosophical;
        if (toneString == "urgent" || toneString == "hurried") return DialogueTone::Urgent;
        if (toneString == "secretive" || toneString == "whispered") return DialogueTone::Secretive;

        return DialogueTone::Neutral;
    }

    void DialogueSystem::UpdateRelationshipFromChoice(const String& npcId, const DialogueChoice& choice) {
        NPCController* npc = m_npcManager->GetNPC(npcId);
        if (!npc || choice.relationshipImpact == 0.0f) {
            return;
        }

        F32 impact = choice.relationshipImpact;
        F32 trustChange = impact * 0.6f;
        F32 respectChange = impact * 0.4f;

        npc->UpdateRelationship(trustChange, respectChange);
        TrackRelationshipChange(npcId, impact);

        AGK_DEBUG("DialogueSystem: Updated relationship for NPC '{0}' by {1:.2f}", npcId, impact);
    }

    void DialogueSystem::TrackRelationshipChange(const String& npcId, F32 change) {
        m_relationshipCache[npcId] += change;

        if (m_activeDialogue && m_activeDialogue->npcId == npcId) {
            m_activeDialogue->relationshipChangeThisConversation += change;
        }
    }

    F32 DialogueSystem::CalculateRelationshipImpact(const String& choiceType, const String& npcId) const {
        // Base impact by choice type
        F32 baseImpact = 0.0f;

        if (choiceType == "respectful" || choiceType == "polite") {
            baseImpact = 0.1f;
        }
        else if (choiceType == "helpful" || choiceType == "supportive") {
            baseImpact = 0.15f;
        }
        else if (choiceType == "aggressive" || choiceType == "rude") {
            baseImpact = -0.2f;
        }
        else if (choiceType == "dismissive" || choiceType == "insulting") {
            baseImpact = -0.25f;
        }
        else if (choiceType == "diplomatic") {
            baseImpact = 0.08f;
        }
        else if (choiceType == "questioning") {
            baseImpact = 0.02f;
        }

        // Faction-specific modifiers
        NPCController* npc = m_npcManager->GetNPC(npcId);
        if (npc) {
            const NPCComponent& npcData = npc->GetNPCData();

            // Apply personality modifiers
            if (choiceType == "aggressive" && npcData.personality.aggressiveness > 0.7f) {
                baseImpact *= 0.5f; // Aggressive NPCs are less offended by aggression
            }

            if (choiceType == "philosophical" && npcData.faction == NPCFaction::Ashvattha) {
                baseImpact *= 1.5f; // Ashvattha loves philosophical discussion
            }

            if (choiceType == "logical" && npcData.faction == NPCFaction::Vaikuntha) {
                baseImpact *= 1.3f; // Vaikuntha appreciates logic
            }

            if (choiceType == "rebellious" && npcData.faction == NPCFaction::YugaStriders) {
                baseImpact *= 1.2f; // Yuga Striders like rebellious spirit
            }
        }

        return Clamp(baseImpact, -1.0f, 1.0f);
    }

    void DialogueSystem::AddExchange(const String& speaker, const String& message, const String& tone) {
        if (!m_activeDialogue) {
            return;
        }

        DialogueExchange exchange;
        exchange.timestamp = std::chrono::steady_clock::now();
        exchange.speaker = speaker;
        exchange.message = SanitizeMessage(message);
        exchange.emotionalTone = tone;
        exchange.relationshipImpact = 0.0f; // Will be calculated separately

        // Extract topics from message (simplified)
        if (message.find("philosophy") != String::npos) exchange.topics.push_back("philosophy");
        if (message.find("technology") != String::npos) exchange.topics.push_back("technology");
        if (message.find("freedom") != String::npos) exchange.topics.push_back("freedom");
        if (message.find("tradition") != String::npos) exchange.topics.push_back("tradition");

        m_activeDialogue->exchanges.push_back(exchange);
        m_activeDialogue->lastExchangeTime = exchange.timestamp;

        AGK_DEBUG("DialogueSystem: Added exchange from {0}: '{1}'", speaker, message);
    }

    void DialogueSystem::UpdateConversationMetrics() {
        if (!m_activeDialogue) {
            return;
        }

        // Update any real-time metrics here
        // For now, most metrics are updated when the conversation ends
    }

    void DialogueSystem::CleanupOldConversations() {
        // Remove conversation history older than a certain threshold
        // This is a placeholder - in a real implementation, you might want to
        // keep important conversations or summarize old ones

        const size_t MAX_CONVERSATIONS_PER_NPC = 5;

        for (auto& [npcId, history] : m_conversationHistory) {
            if (history.size() > MAX_CONVERSATIONS_PER_NPC) {
                // Keep only the most recent conversations
                std::vector<DialogueExchange> recentHistory(
                    history.end() - MAX_CONVERSATIONS_PER_NPC,
                    history.end()
                );
                history = recentHistory;
            }
        }
    }

    bool DialogueSystem::ValidateNPCForConversation(const String& npcId) const {
        NPCController* npc = m_npcManager->GetNPC(npcId);
        if (!npc) {
            return false;
        }

        if (!npc->IsActive()) {
            AGK_WARN("DialogueSystem: NPC '{0}' is not active", npcId);
            return false;
        }

        if (!npc->CanInteractWithPlayer()) {
            AGK_WARN("DialogueSystem: Cannot interact with NPC '{0}' in current state", npcId);
            return false;
        }

        return true;
    }

    bool DialogueSystem::ValidateChoiceIndex(U32 choiceIndex) const {
        if (!m_activeDialogue) {
            return false;
        }

        return choiceIndex < m_activeDialogue->currentChoices.size();
    }

    void DialogueSystem::HandleConversationError(const String& error) {
        AGK_ERROR("DialogueSystem: Conversation error - {0}", error);

        if (m_activeDialogue) {
            EndConversation(DialogueEndReason::Error);
        }
    }

    void DialogueSystem::LogConversationError(const String& operation, const String& error) const {
        AGK_ERROR("DialogueSystem: {0} failed - {1}", operation, error);
    }

    void DialogueSystem::OptimizeChoiceGeneration() {
        // Remove old cache entries
        const size_t MAX_CACHE_SIZE = 100;
        if (m_choiceCache.size() > MAX_CACHE_SIZE) {
            // Clear half the cache (simple strategy)
            auto it = m_choiceCache.begin();
            std::advance(it, m_choiceCache.size() / 2);
            m_choiceCache.erase(m_choiceCache.begin(), it);
        }
    }

    void DialogueSystem::CacheFrequentChoices() {
        // This could analyze usage patterns and pre-cache common choices
        // For now, it's a placeholder
    }

    void DialogueSystem::PurgeOldCaches() {
        OptimizeChoiceGeneration();

        // Clear relationship cache for NPCs not recently interacted with
        const size_t MAX_RELATIONSHIP_CACHE = 50;
        if (m_relationshipCache.size() > MAX_RELATIONSHIP_CACHE) {
            m_relationshipCache.clear();
        }
    }

    String DialogueSystem::GetTimeStampString() const {
        auto now = std::chrono::steady_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        std::stringstream ss;
        struct tm timeinfo_new;
        localtime_s(&timeinfo_new, &time_t);
        ss << std::put_time(&timeinfo_new, "%H:%M:%S");
        return ss.str();
    }

    String DialogueSystem::SanitizeMessage(const String& message) const {
        // Remove any problematic characters or formatting
        String sanitized = message;

        // Remove excessive whitespace
        while (sanitized.find("  ") != String::npos) {
            sanitized.replace(sanitized.find("  "), 2, " ");
        }

        // Trim leading/trailing whitespace
        sanitized.erase(0, sanitized.find_first_not_of(" \t"));
        sanitized.erase(sanitized.find_last_not_of(" \t") + 1);

        return sanitized;
    }

    F32 DialogueSystem::CalculateTypingDuration(const String& message) const {
        if (!m_settings.enableTypingEffect) {
            return 0.0f;
        }

        return message.length() / m_settings.messageDisplaySpeed;
    }

    DialogueTone DialogueSystem::GetDefaultToneForFaction(NPCFaction faction) const {
        switch (faction) {
        case NPCFaction::Ashvattha:
            return DialogueTone::Philosophical;
        case NPCFaction::Vaikuntha:
            return DialogueTone::Respectful;
        case NPCFaction::YugaStriders:
            return DialogueTone::Friendly;
        default:
            return DialogueTone::Neutral;
        }
    }

} // namespace Angaraka::AI