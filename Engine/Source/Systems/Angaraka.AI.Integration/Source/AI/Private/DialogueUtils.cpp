#include <Angaraka/AIBase.hpp>
#include "Angaraka/DialogueSystem.hpp"
#include <sstream>
#include <regex>

namespace Angaraka::AI::DialogueUtils {

    // ==================================================================================
    // Message Formatting
    // ==================================================================================

    String FormatNPCMessage(const String& npcName, const String& message, DialogueTone tone) {
        std::stringstream formatted;

        // Add speaker name with tone indication
        formatted << npcName;

        // Add tone-specific formatting
        switch (tone) {
        case DialogueTone::Friendly:
            formatted << " (warmly)";
            break;
        case DialogueTone::Hostile:
            formatted << " (angrily)";
            break;
        case DialogueTone::Respectful:
            formatted << " (respectfully)";
            break;
        case DialogueTone::Dismissive:
            formatted << " (dismissively)";
            break;
        case DialogueTone::Philosophical:
            formatted << " (thoughtfully)";
            break;
        case DialogueTone::Urgent:
            formatted << " (urgently)";
            break;
        case DialogueTone::Secretive:
            formatted << " (quietly)";
            break;
        default:
            // No tone modifier for neutral
            break;
        }

        formatted << ": " << message;

        return formatted.str();
    }

    String FormatPlayerChoice(const DialogueChoice& choice, U32 index) {
        std::stringstream formatted;

        // Add choice number
        formatted << "[" << (index + 1) << "] ";

        // Add availability indicator
        if (!choice.isAvailable) {
            formatted << "(Unavailable) ";
        }
        else if (choice.requiresCheck) {
            formatted << "(" << choice.checkType << ") ";
        }

        // Add the choice text
        formatted << choice.choiceText;

        // Add relationship impact hint if significant
        if (choice.relationshipImpact > 0.1f) {
            formatted << " [+]";
        }
        else if (choice.relationshipImpact < -0.1f) {
            formatted << " [-]";
        }

        return formatted.str();
    }

    String FormatRelationshipChange(F32 change) {
        if (change == 0.0f) {
            return "";
        }

        std::stringstream formatted;

        if (change > 0.0f) {
            formatted << "Relationship improved";
            if (change > 0.2f) {
                formatted << " significantly";
            }
            else if (change > 0.1f) {
                formatted << " moderately";
            }
            else {
                formatted << " slightly";
            }
            formatted << " (+" << std::fixed << std::setprecision(2) << change << ")";
        }
        else {
            formatted << "Relationship worsened";
            if (change < -0.2f) {
                formatted << " significantly";
            }
            else if (change < -0.1f) {
                formatted << " moderately";
            }
            else {
                formatted << " slightly";
            }
            formatted << " (" << std::fixed << std::setprecision(2) << change << ")";
        }

        return formatted.str();
    }

    // ==================================================================================
    // Choice Analysis
    // ==================================================================================

    std::vector<String> ExtractTopicsFromMessage(const String& message) {
        std::vector<String> topics;
        String lowerMessage = message;
        std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);

        // Define topic keywords for each faction and general themes
        std::unordered_map<String, std::vector<String>> topicKeywords = {
            {"philosophy", {"philosophy", "wisdom", "truth", "meaning", "existence", "consciousness", "meditation", "enlightenment"}},
            {"technology", {"technology", "tech", "system", "algorithm", "data", "efficiency", "optimization", "innovation", "progress"}},
            {"freedom", {"freedom", "liberty", "independence", "choice", "rebellion", "resistance", "oppression", "justice"}},
            {"tradition", {"tradition", "custom", "ancient", "heritage", "culture", "ritual", "ceremony", "history", "ancestors"}},
            {"spirituality", {"spiritual", "divine", "sacred", "holy", "blessed", "prayer", "faith", "belief", "soul"}},
            {"science", {"science", "research", "experiment", "discovery", "theory", "hypothesis", "evidence", "analysis"}},
            {"politics", {"politics", "government", "authority", "power", "control", "leadership", "faction", "alliance"}},
            {"economics", {"economics", "trade", "money", "wealth", "resources", "market", "economy", "business", "profit"}},
            {"relationships", {"friend", "enemy", "ally", "trust", "respect", "love", "hate", "family", "loyalty"}},
            {"conflict", {"war", "battle", "fight", "conflict", "violence", "peace", "treaty", "negotiation", "compromise"}},
            {"knowledge", {"knowledge", "learning", "education", "study", "research", "information", "understanding", "insight"}},
            {"morality", {"right", "wrong", "good", "evil", "moral", "ethical", "virtue", "sin", "honor", "duty"}}
        };

        // Check for each topic
        for (const auto& [topic, keywords] : topicKeywords) {
            for (const String& keyword : keywords) {
                if (lowerMessage.find(keyword) != String::npos) {
                    // Avoid duplicate topics
                    if (std::find(topics.begin(), topics.end(), topic) == topics.end()) {
                        topics.push_back(topic);
                    }
                    break; // Found this topic, move to next
                }
            }
        }

        // If no specific topics found, determine from message structure
        if (topics.empty()) {
            if (lowerMessage.find("?") != String::npos) {
                topics.push_back("questioning");
            }
            else if (lowerMessage.find("!") != String::npos) {
                topics.push_back("emphatic");
            }
            else {
                topics.push_back("general");
            }
        }

        return topics;
    }

    String DetermineChoiceType(const String& choiceText) {
        String lowerChoice = choiceText;
        std::transform(lowerChoice.begin(), lowerChoice.end(), lowerChoice.begin(), ::tolower);

        // Aggressive indicators
        std::vector<String> aggressiveKeywords = {
            "fight", "attack", "threaten", "force", "demand", "insist", "angry", "furious", "rage"
        };

        // Diplomatic indicators
        std::vector<String> diplomaticKeywords = {
            "negotiate", "discuss", "compromise", "understand", "respect", "appreciate", "consider"
        };

        // Questioning indicators
        std::vector<String> questioningKeywords = {
            "why", "how", "what", "when", "where", "who", "explain", "tell me", "ask", "question"
        };

        // Respectful indicators
        std::vector<String> respectfulKeywords = {
            "please", "thank you", "honor", "privilege", "grateful", "appreciate", "respect"
        };

        // Dismissive indicators
        std::vector<String> dismissiveKeywords = {
            "whatever", "don't care", "boring", "waste of time", "ridiculous", "stupid", "pointless"
        };

        // Supportive indicators
        std::vector<String> supportiveKeywords = {
            "agree", "support", "help", "assist", "ally", "together", "cooperate", "join"
        };

        // Philosophical indicators
        std::vector<String> philosophicalKeywords = {
            "meaning", "purpose", "truth", "wisdom", "existence", "consciousness", "reality", "nature"
        };

        // Logical indicators
        std::vector<String> logicalKeywords = {
            "logical", "rational", "reason", "evidence", "proof", "fact", "analyze", "conclude"
        };

        // Check each category
        for (const String& keyword : aggressiveKeywords) {
            if (lowerChoice.find(keyword) != String::npos) {
                return "aggressive";
            }
        }

        for (const String& keyword : diplomaticKeywords) {
            if (lowerChoice.find(keyword) != String::npos) {
                return "diplomatic";
            }
        }

        for (const String& keyword : questioningKeywords) {
            if (lowerChoice.find(keyword) != String::npos) {
                return "questioning";
            }
        }

        for (const String& keyword : respectfulKeywords) {
            if (lowerChoice.find(keyword) != String::npos) {
                return "respectful";
            }
        }

        for (const String& keyword : dismissiveKeywords) {
            if (lowerChoice.find(keyword) != String::npos) {
                return "dismissive";
            }
        }

        for (const String& keyword : supportiveKeywords) {
            if (lowerChoice.find(keyword) != String::npos) {
                return "supportive";
            }
        }

        for (const String& keyword : philosophicalKeywords) {
            if (lowerChoice.find(keyword) != String::npos) {
                return "philosophical";
            }
        }

        for (const String& keyword : logicalKeywords) {
            if (lowerChoice.find(keyword) != String::npos) {
                return "logical";
            }
        }

        // Check punctuation for additional hints
        if (choiceText.find("?") != String::npos) {
            return "questioning";
        }
        else if (choiceText.find("!") != String::npos) {
            return "emphatic";
        }
        else if (choiceText.find("...") != String::npos) {
            return "hesitant";
        }

        return "neutral";
    }

    F32 EstimateChoiceDifficulty(const String& choiceType, NPCFaction targetFaction) {
        F32 baseDifficulty = 0.5f; // Default moderate difficulty

        // Base difficulty by choice type
        if (choiceType == "aggressive" || choiceType == "threatening") {
            baseDifficulty = 0.8f; // High risk
        }
        else if (choiceType == "diplomatic" || choiceType == "respectful") {
            baseDifficulty = 0.3f; // Usually safe
        }
        else if (choiceType == "philosophical") {
            baseDifficulty = 0.6f; // Requires knowledge
        }
        else if (choiceType == "questioning") {
            baseDifficulty = 0.2f; // Generally safe
        }
        else if (choiceType == "dismissive" || choiceType == "insulting") {
            baseDifficulty = 0.9f; // Very risky
        }
        else if (choiceType == "logical") {
            baseDifficulty = 0.4f; // Usually reasonable
        }
        else if (choiceType == "supportive") {
            baseDifficulty = 0.25f; // Generally positive
        }

        // Faction-specific adjustments
        switch (targetFaction) {
        case NPCFaction::Ashvattha:
            // Traditional faction appreciates respect and philosophy
            if (choiceType == "respectful" || choiceType == "philosophical") {
                baseDifficulty *= 0.7f; // Easier
            }
            else if (choiceType == "aggressive" || choiceType == "dismissive") {
                baseDifficulty *= 1.3f; // Harder
            }
            break;

        case NPCFaction::Vaikuntha:
            // Tech faction appreciates logic and efficiency
            if (choiceType == "logical" || choiceType == "analytical") {
                baseDifficulty *= 0.6f; // Easier
            }
            else if (choiceType == "emotional" || choiceType == "irrational") {
                baseDifficulty *= 1.4f; // Harder
            }
            break;

        case NPCFaction::YugaStriders:
            // Rebel faction appreciates authenticity and passion
            if (choiceType == "passionate" || choiceType == "rebellious") {
                baseDifficulty *= 0.8f; // Easier
            }
            else if (choiceType == "authoritarian" || choiceType == "submissive") {
                baseDifficulty *= 1.2f; // Harder
            }
            break;

        case NPCFaction::Neutral:
            // Neutral faction is generally balanced
            break;

        default:
            break;
        }

        return Clamp(baseDifficulty, 0.1f, 1.0f);
    }

    // ==================================================================================
    // Conversation Analysis
    // ==================================================================================

    String SummarizeConversation(const std::vector<DialogueExchange>& exchanges) {
        if (exchanges.empty()) {
            return "No conversation took place.";
        }

        std::stringstream summary;

        // Basic statistics
        U32 playerMessages = 0;
        U32 npcMessages = 0;
        std::unordered_map<String, U32> topicCounts;
        std::unordered_map<String, U32> toneCounts;
        F32 totalRelationshipImpact = 0.0f;

        for (const auto& exchange : exchanges) {
            if (exchange.speaker == "player") {
                playerMessages++;
            }
            else {
                npcMessages++;
                toneCounts[exchange.emotionalTone]++;
            }

            // Count topics
            for (const String& topic : exchange.topics) {
                topicCounts[topic]++;
            }

            totalRelationshipImpact += exchange.relationshipImpact;
        }

        // Generate summary
        summary << "Conversation Summary:\n";
        summary << "Exchanges: " << exchanges.size() << " (Player: " << playerMessages << ", NPC: " << npcMessages << ")\n";

        // Most discussed topics
        if (!topicCounts.empty()) {
            auto maxTopic = std::max_element(topicCounts.begin(), topicCounts.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            summary << "Main topic: " << maxTopic->first << " (" << maxTopic->second << " mentions)\n";
        }

        // Overall tone
        if (!toneCounts.empty()) {
            auto maxTone = std::max_element(toneCounts.begin(), toneCounts.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            summary << "NPC tone: " << maxTone->first << "\n";
        }

        // Relationship impact
        if (totalRelationshipImpact != 0.0f) {
            summary << "Relationship change: " << (totalRelationshipImpact > 0 ? "+" : "")
                << std::fixed << std::setprecision(2) << totalRelationshipImpact << "\n";
        }

        // Conversation outcome
        if (totalRelationshipImpact > 0.2f) {
            summary << "Outcome: Positive interaction";
        }
        else if (totalRelationshipImpact < -0.2f) {
            summary << "Outcome: Negative interaction";
        }
        else {
            summary << "Outcome: Neutral interaction";
        }

        return summary.str();
    }

    F32 CalculateConversationTension(const std::vector<DialogueExchange>& exchanges) {
        if (exchanges.empty()) {
            return 0.0f;
        }

        F32 tension = 0.0f;
        F32 weight = 1.0f;

        for (const auto& exchange : exchanges) {
            F32 exchangeTension = 0.0f;

            // Tension from emotional tone
            if (exchange.emotionalTone == "hostile" || exchange.emotionalTone == "angry") {
                exchangeTension += 0.8f;
            }
            else if (exchange.emotionalTone == "dismissive" || exchange.emotionalTone == "condescending") {
                exchangeTension += 0.6f;
            }
            else if (exchange.emotionalTone == "urgent" || exchange.emotionalTone == "worried") {
                exchangeTension += 0.4f;
            }
            else if (exchange.emotionalTone == "friendly" || exchange.emotionalTone == "calm") {
                exchangeTension -= 0.3f;
            }

            // Tension from relationship impact
            if (exchange.relationshipImpact < -0.1f) {
                exchangeTension += Abs(exchange.relationshipImpact) * 2.0f;
            }
            else if (exchange.relationshipImpact > 0.1f) {
                exchangeTension -= exchange.relationshipImpact;
            }

            // Weight recent exchanges more heavily
            tension += exchangeTension * weight;
            weight *= 0.9f; // Decay weight for older exchanges
        }

        // Normalize by number of exchanges
        tension /= exchanges.size();

        return Clamp(tension, 0.0f, 1.0f);
    }

    std::vector<String> GetFrequentTopics(const std::vector<DialogueExchange>& exchanges) {
        std::unordered_map<String, U32> topicCounts;

        // Count all topics across exchanges
        for (const auto& exchange : exchanges) {
            for (const String& topic : exchange.topics) {
                topicCounts[topic]++;
            }
        }

        // Sort topics by frequency
        std::vector<std::pair<String, U32>> sortedTopics(topicCounts.begin(), topicCounts.end());
        std::sort(sortedTopics.begin(), sortedTopics.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        // Return top topics
        std::vector<String> frequentTopics;
        const size_t maxTopics = 5;

        for (size_t i = 0; i < Min(maxTopics, sortedTopics.size()); ++i) {
            if (sortedTopics[i].second >= 2) { // Only include topics mentioned at least twice
                frequentTopics.push_back(sortedTopics[i].first);
            }
        }

        return frequentTopics;
    }

    // ==================================================================================
    // Validation Helpers
    // ==================================================================================

    bool IsValidChoiceText(const String& text) {
        if (text.empty()) {
            return false;
        }

        // Check minimum and maximum length
        if (text.length() < 3 || text.length() > 200) {
            return false;
        }

        // Check for inappropriate content patterns
        String lowerText = text;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);

        // Simple profanity filter (basic implementation)
        std::vector<String> inappropriate = {
            "damn", "hell", "bastard", "stupid", "idiot", "moron"
            // Note: In a real implementation, this would be more comprehensive
        };

        for (const String& word : inappropriate) {
            if (lowerText.find(word) != String::npos) {
                AGK_WARN("DialogueUtils: Choice contains inappropriate content: '{0}'", text);
                return false;
            }
        }

        // Check for excessive punctuation
        U32 punctuationCount = 0;
        for (char c : text) {
            if (c == '!' || c == '?' || c == '.' || c == ',') {
                punctuationCount++;
            }
        }

        if (punctuationCount > text.length() / 4) {
            return false; // Too much punctuation
        }

        return true;
    }

    bool IsAppropriateForFaction(const String& message, NPCFaction faction) {
        String lowerMessage = message;
        std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);

        switch (faction) {
        case NPCFaction::Ashvattha:
            // Traditional faction - check for respectful language
            if (lowerMessage.find("sacred") != String::npos ||
                lowerMessage.find("wisdom") != String::npos ||
                lowerMessage.find("tradition") != String::npos) {
                return true;
            }

            // Inappropriate for Ashvattha
            if (lowerMessage.find("technology sucks") != String::npos ||
                lowerMessage.find("old fashioned") != String::npos ||
                lowerMessage.find("backwards") != String::npos) {
                return false;
            }
            break;

        case NPCFaction::Vaikuntha:
            // Tech faction - check for logical/progressive language
            if (lowerMessage.find("efficient") != String::npos ||
                lowerMessage.find("logical") != String::npos ||
                lowerMessage.find("progress") != String::npos) {
                return true;
            }

            // Inappropriate for Vaikuntha
            if (lowerMessage.find("technology is evil") != String::npos ||
                lowerMessage.find("primitive") != String::npos ||
                lowerMessage.find("illogical") != String::npos) {
                return false;
            }
            break;

        case NPCFaction::YugaStriders:
            // Rebel faction - check for freedom/justice language
            if (lowerMessage.find("freedom") != String::npos ||
                lowerMessage.find("justice") != String::npos ||
                lowerMessage.find("rebellion") != String::npos) {
                return true;
            }

            // Inappropriate for YugaStriders
            if (lowerMessage.find("submit") != String::npos ||
                lowerMessage.find("obey") != String::npos ||
                lowerMessage.find("authority is right") != String::npos) {
                return false;
            }
            break;

        case NPCFaction::Neutral:
            // Neutral faction is generally accepting
            return true;

        default:
            break;
        }

        return true; // Default to appropriate if no specific issues found
    }

    bool RequiresContentWarning(const String& message) {
        String lowerMessage = message;
        std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);

        // Topics that might require content warnings
        std::vector<String> sensitiveTopics = {
            "violence", "death", "murder", "kill", "suicide", "torture",
            "war", "genocide", "slavery", "abuse", "trauma",
            "sexual", "explicit", "graphic", "disturbing"
        };

        for (const String& topic : sensitiveTopics) {
            if (lowerMessage.find(topic) != String::npos) {
                return true;
            }
        }

        return false;
    }

    // ==================================================================================
    // UI Integration Helpers
    // ==================================================================================

    String ConvertToDisplayText(const String& rawText) {
        String displayText = rawText;

        // Replace markdown-style formatting with display equivalents
        // **bold** -> [B]bold[/B]
        std::regex boldRegex(R"(\*\*(.*?)\*\*)");
        displayText = std::regex_replace(displayText, boldRegex, "[B]$1[/B]");

        // *italic* -> [I]italic[/I]
        std::regex italicRegex(R"(\*(.*?)\*)");
        displayText = std::regex_replace(displayText, italicRegex, "[I]$1[/I]");

        // _underline_ -> [U]underline[/U]
        std::regex underlineRegex(R"(_(.*?)_)");
        displayText = std::regex_replace(displayText, underlineRegex, "[U]$1[/U]");

        // Convert special characters
        std::unordered_map<String, String> replacements = {
            {"&", "&amp;"},
            {"<", "&lt;"},
            {">", "&gt;"},
            {"\"", "&quot;"},
            {"'", "&#39;"}
        };

        for (const auto& [from, to] : replacements) {
            size_t pos = 0;
            while ((pos = displayText.find(from, pos)) != String::npos) {
                displayText.replace(pos, from.length(), to);
                pos += to.length();
            }
        }

        return displayText;
    }

    std::vector<String> WrapTextForUI(const String& text, U32 maxLineLength) {
        std::vector<String> lines;

        if (text.empty()) {
            return lines;
        }

        std::stringstream ss(text);
        String word;
        String currentLine;

        while (ss >> word) {
            // Check if adding this word would exceed the line length
            if (!currentLine.empty() && (currentLine.length() + word.length() + 1) > maxLineLength) {
                // Save current line and start a new one
                lines.push_back(currentLine);
                currentLine = word;
            }
            else {
                // Add word to current line
                if (!currentLine.empty()) {
                    currentLine += " ";
                }
                currentLine += word;
            }
        }

        // Don't forget the last line
        if (!currentLine.empty()) {
            lines.push_back(currentLine);
        }

        // Handle case where no words were found (shouldn't happen with normal text)
        if (lines.empty()) {
            lines.push_back(text);
        }

        return lines;
    }

    String GetFactionColorCode(NPCFaction faction) {
        switch (faction) {
        case NPCFaction::Ashvattha:
            return "#8B4513"; // Saddle Brown - representing earth, tradition, spirituality

        case NPCFaction::Vaikuntha:
            return "#4169E1"; // Royal Blue - representing technology, logic, progress

        case NPCFaction::YugaStriders:
            return "#DC143C"; // Crimson - representing rebellion, passion, change

        case NPCFaction::Neutral:
            return "#808080"; // Gray - representing neutrality, balance

        default:
            return "#FFFFFF"; // White - default fallback
        }
    }

    // ==================================================================================
    // Advanced Analysis Functions
    // ==================================================================================

    String AnalyzeConversationStyle(const std::vector<DialogueExchange>& exchanges) {
        if (exchanges.empty()) {
            return "No conversation data";
        }

        std::unordered_map<String, U32> styleMetrics;

        for (const auto& exchange : exchanges) {
            if (exchange.speaker == "player") {
                continue; // Focus on NPC style
            }

            String message = exchange.message;
            std::transform(message.begin(), message.end(), message.begin(), ::tolower);

            // Analyze linguistic patterns
            if (message.find("perhaps") != String::npos ||
                message.find("consider") != String::npos ||
                message.find("ponder") != String::npos) {
                styleMetrics["philosophical"]++;
            }

            if (message.find("data") != String::npos ||
                message.find("analysis") != String::npos ||
                message.find("conclude") != String::npos) {
                styleMetrics["analytical"]++;
            }

            if (message.find("feel") != String::npos ||
                message.find("heart") != String::npos ||
                message.find("emotion") != String::npos) {
                styleMetrics["emotional"]++;
            }

            if (message.find("must") != String::npos ||
                message.find("should") != String::npos ||
                message.find("necessary") != String::npos) {
                styleMetrics["directive"]++;
            }

            // Count questions
            if (message.find("?") != String::npos) {
                styleMetrics["questioning"]++;
            }

            // Count exclamations
            if (message.find("!") != String::npos) {
                styleMetrics["emphatic"]++;
            }
        }

        // Determine dominant style
        if (styleMetrics.empty()) {
            return "Neutral conversational style";
        }

        auto maxStyle = std::max_element(styleMetrics.begin(), styleMetrics.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

        return "Predominantly " + maxStyle->first + " conversational style";
    }

    F32 CalculateDialogueComplexity(const std::vector<DialogueExchange>& exchanges) {
        if (exchanges.empty()) {
            return 0.0f;
        }

        F32 totalComplexity = 0.0f;
        U32 npcMessageCount = 0;

        for (const auto& exchange : exchanges) {
            if (exchange.speaker == "player") {
                continue; // Focus on NPC dialogue complexity
            }

            npcMessageCount++;
            F32 messageComplexity = 0.0f;

            // Word count complexity
            std::stringstream ss(exchange.message);
            String word;
            U32 wordCount = 0;
            U32 longWordCount = 0;

            while (ss >> word) {
                wordCount++;
                if (word.length() > 6) {
                    longWordCount++;
                }
            }

            // Base complexity from length
            messageComplexity += Min(wordCount / 20.0f, 1.0f) * 0.3f;

            // Complexity from vocabulary
            messageComplexity += (static_cast<F32>(longWordCount) / wordCount) * 0.4f;

            // Complexity from sentence structure
            U32 sentenceCount = static_cast<U32>(std::count(exchange.message.begin(), exchange.message.end(), '.') +
                std::count(exchange.message.begin(), exchange.message.end(), '!') +
                std::count(exchange.message.begin(), exchange.message.end(), '?'));

            if (sentenceCount > 0) {
                F32 avgWordsPerSentence = static_cast<F32>(wordCount) / sentenceCount;
                messageComplexity += Min(avgWordsPerSentence / 15.0f, 1.0f) * 0.2f;
            }

            // Complexity from topics discussed
            messageComplexity += Min(static_cast<F32>(exchange.topics.size()) / 3.0f, 1.0f) * 0.1f;

            totalComplexity += messageComplexity;
        }

        return npcMessageCount > 0 ? totalComplexity / npcMessageCount : 0.0f;
    }

    std::unordered_map<String, F32> AnalyzeEmotionalProgression(const std::vector<DialogueExchange>& exchanges) {
        std::unordered_map<String, F32> emotionalMetrics;

        if (exchanges.empty()) {
            return emotionalMetrics;
        }

        // Map emotional tones to numerical values
        std::unordered_map<String, F32> toneValues = {
            {"hostile", -0.8f}, {"angry", -0.7f}, {"dismissive", -0.5f},
            {"condescending", -0.4f}, {"skeptical", -0.2f}, {"neutral", 0.0f},
            {"curious", 0.2f}, {"respectful", 0.4f}, {"friendly", 0.6f},
            {"warm", 0.7f}, {"enthusiastic", 0.8f}, {"philosophical", 0.3f},
            {"contemplative", 0.1f}, {"urgent", -0.1f}, {"secretive", -0.3f}
        };

        F32 initialTone = 0.0f;
        F32 finalTone = 0.0f;
        F32 peakPositive = -1.0f;
        F32 peakNegative = 1.0f;
        F32 volatility = 0.0f;

        std::vector<F32> toneProgression;

        for (const auto& exchange : exchanges) {
            if (exchange.speaker == "player") {
                continue; // Focus on NPC emotional state
            }

            F32 toneValue = 0.0f;
            auto toneIt = toneValues.find(exchange.emotionalTone);
            if (toneIt != toneValues.end()) {
                toneValue = toneIt->second;
            }

            toneProgression.push_back(toneValue);
            peakPositive = Max(peakPositive, toneValue);
            peakNegative = Min(peakNegative, toneValue);
        }

        if (!toneProgression.empty()) {
            initialTone = toneProgression.front();
            finalTone = toneProgression.back();

            // Calculate volatility (how much the tone changes)
            for (size_t i = 1; i < toneProgression.size(); ++i) {
                volatility += Abs(toneProgression[i] - toneProgression[i - 1]);
            }
            volatility /= Max(1.0f, static_cast<F32>(toneProgression.size() - 1));
        }

        emotionalMetrics["initial_tone"] = initialTone;
        emotionalMetrics["final_tone"] = finalTone;
        emotionalMetrics["tone_change"] = finalTone - initialTone;
        emotionalMetrics["peak_positive"] = peakPositive;
        emotionalMetrics["peak_negative"] = peakNegative;
        emotionalMetrics["emotional_range"] = peakPositive - peakNegative;
        emotionalMetrics["volatility"] = volatility;

        return emotionalMetrics;
    }

    String GenerateConversationInsights(const std::vector<DialogueExchange>& exchanges) {
        if (exchanges.empty()) {
            return "No conversation data available for analysis.";
        }

        std::stringstream insights;
        insights << "=== Conversation Analysis ===\n\n";

        // Basic metrics
        U32 playerTurns = 0;
        U32 npcTurns = 0;
        for (const auto& exchange : exchanges) {
            if (exchange.speaker == "player") {
                playerTurns++;
            }
            else {
                npcTurns++;
            }
        }

        insights << "Participation: Player (" << playerTurns << " turns), NPC (" << npcTurns << " turns)\n";

        // Conversation balance
        if (playerTurns > npcTurns * 1.5f) {
            insights << "Player dominated the conversation.\n";
        }
        else if (npcTurns > playerTurns * 1.5f) {
            insights << "NPC dominated the conversation.\n";
        }
        else {
            insights << "Balanced conversation participation.\n";
        }

        // Topic analysis
        auto topics = GetFrequentTopics(exchanges);
        if (!topics.empty()) {
            insights << "Main topics discussed: ";
            for (size_t i = 0; i < topics.size(); ++i) {
                insights << topics[i];
                if (i < topics.size() - 1) insights << ", ";
            }
            insights << "\n";
        }

        // Emotional analysis
        auto emotional = AnalyzeEmotionalProgression(exchanges);
        if (!emotional.empty()) {
            F32 toneChange = emotional["tone_change"];
            if (toneChange > 0.2f) {
                insights << "NPC mood improved significantly during conversation.\n";
            }
            else if (toneChange < -0.2f) {
                insights << "NPC mood worsened during conversation.\n";
            }
            else {
                insights << "NPC mood remained relatively stable.\n";
            }

            F32 volatility = emotional["volatility"];
            if (volatility > 0.4f) {
                insights << "Highly emotional conversation with significant mood swings.\n";
            }
            else if (volatility > 0.2f) {
                insights << "Moderately emotional conversation.\n";
            }
            else {
                insights << "Calm, stable emotional tone throughout.\n";
            }
        }

        // Complexity analysis
        F32 complexity = CalculateDialogueComplexity(exchanges);
        if (complexity > 0.7f) {
            insights << "Highly sophisticated dialogue with complex topics.\n";
        }
        else if (complexity > 0.4f) {
            insights << "Moderately complex conversation.\n";
        }
        else {
            insights << "Simple, straightforward conversation.\n";
        }

        // Conversation style
        String style = AnalyzeConversationStyle(exchanges);
        insights << style << "\n";

        // Tension analysis
        F32 tension = CalculateConversationTension(exchanges);
        if (tension > 0.6f) {
            insights << "High tension conversation - conflict or disagreement present.\n";
        }
        else if (tension > 0.3f) {
            insights << "Some tension or disagreement in the conversation.\n";
        }
        else {
            insights << "Low tension - cooperative or friendly exchange.\n";
        }

        return insights.str();
    }

    // ==================================================================================
    // Utility Functions for Special Cases
    // ==================================================================================

    String HandleSpecialCharacters(const String& text) {
        String processed = text;

        // Handle unicode characters that might cause issues
        std::unordered_map<String, String> unicodeReplacements = {
            {"â€™", "'"},    // Smart apostrophe
            {"â€œ", "\""},   // Smart quote open
            {"â€�", "\""},   // Smart quote close
            {"â€”", " - "},  // Em dash
            {"â€“", " - "},  // En dash
            {"â€¦", "..."}   // Ellipsis
        };

        for (const auto& [unicode, replacement] : unicodeReplacements) {
            size_t pos = 0;
            while ((pos = processed.find(unicode, pos)) != String::npos) {
                processed.replace(pos, unicode.length(), replacement);
                pos += replacement.length();
            }
        }

        return processed;
    }

    bool IsEmergencyExit(const String& message) {
        String lowerMessage = message;
        std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);

        std::vector<String> emergencyPhrases = {
            "emergency exit", "need to leave", "must go now", "urgent matter",
            "emergency", "crisis", "help needed", "trouble"
        };

        for (const String& phrase : emergencyPhrases) {
            if (lowerMessage.find(phrase) != String::npos) {
                return true;
            }
        }

        return false;
    }

    String SanitizeForDisplay(const String& text) {
        String sanitized = text;

        // Remove or replace potentially harmful content
        sanitized = HandleSpecialCharacters(sanitized);

        // Limit length for display
        const size_t MAX_DISPLAY_LENGTH = 500;
        if (sanitized.length() > MAX_DISPLAY_LENGTH) {
            sanitized = sanitized.substr(0, MAX_DISPLAY_LENGTH - 3) + "...";
        }

        // Remove excessive whitespace
        std::regex excessiveWhitespace(R"(\s+)");
        sanitized = std::regex_replace(sanitized, excessiveWhitespace, " ");

        // Trim leading and trailing whitespace
        sanitized.erase(0, sanitized.find_first_not_of(" \t\n\r"));
        sanitized.erase(sanitized.find_last_not_of(" \t\n\r") + 1);

        return sanitized;
    }

    std::vector<String> ExtractQuotableLines(const std::vector<DialogueExchange>& exchanges) {
        std::vector<String> quotables;

        for (const auto& exchange : exchanges) {
            if (exchange.speaker == "player") {
                continue; // Focus on memorable NPC quotes
            }

            String message = exchange.message;

            // Criteria for quotable lines
            bool isQuotable = false;

            // Philosophical or profound statements
            if (message.find("truth") != String::npos ||
                message.find("wisdom") != String::npos ||
                message.find("meaning") != String::npos ||
                message.find("purpose") != String::npos) {
                isQuotable = true;
            }

            // Strong emotional statements
            if (exchange.emotionalTone == "philosophical" ||
                exchange.emotionalTone == "passionate" ||
                exchange.emotionalTone == "profound") {
                isQuotable = true;
            }

            // Faction-specific memorable phrases
            if (message.find("sacred") != String::npos ||
                message.find("efficiency") != String::npos ||
                message.find("freedom") != String::npos) {
                isQuotable = true;
            }

            // Length criteria (not too short, not too long)
            if (message.length() >= 20 && message.length() <= 150) {
                isQuotable = true;
            }

            if (isQuotable) {
                quotables.push_back(message);
            }
        }

        // Limit to most memorable quotes
        if (quotables.size() > 3) {
            quotables.resize(3);
        }

        return quotables;
    }

    F32 CalculateDialogueQuality(const std::vector<DialogueExchange>& exchanges) {
        if (exchanges.empty()) {
            return 0.0f;
        }

        F32 qualityScore = 0.0f;
        F32 factors = 0.0f;

        // Factor 1: Conversation length (optimal range)
        F32 lengthScore = 0.0f;
        size_t exchangeCount = exchanges.size();
        if (exchangeCount >= 4 && exchangeCount <= 12) {
            lengthScore = 1.0f; // Optimal length
        }
 else if (exchangeCount >= 2 && exchangeCount <= 16) {
  lengthScore = 0.7f; // Acceptable length
}
else {
 lengthScore = 0.3f; // Too short or too long
}
qualityScore += lengthScore;
factors += 1.0f;

// Factor 2: Topic diversity
auto topics = GetFrequentTopics(exchanges);
F32 topicScore = Min(static_cast<F32>(topics.size()) / 3.0f, 1.0f);
qualityScore += topicScore;
factors += 1.0f;

// Factor 3: Emotional engagement
auto emotional = AnalyzeEmotionalProgression(exchanges);
F32 emotionalScore = 0.5f; // Default
if (!emotional.empty()) {
    F32 range = emotional["emotional_range"];
    F32 volatility = emotional["volatility"];

    // Good emotional engagement without excessive volatility
    emotionalScore = Min(range, 1.0f) * 0.7f + Min(1.0f - volatility * 2.0f, 1.0f) * 0.3f;
}
qualityScore += emotionalScore;
factors += 1.0f;

// Factor 4: Dialogue complexity
F32 complexity = CalculateDialogueComplexity(exchanges);
F32 complexityScore = Min(complexity * 1.5f, 1.0f); // Reward complexity but cap it
qualityScore += complexityScore;
factors += 1.0f;

// Factor 5: Tension balance (some tension is good, too much is bad)
F32 tension = CalculateConversationTension(exchanges);
F32 tensionScore = 1.0f - Abs(tension - 0.3f) / 0.7f; // Optimal tension around 0.3
tensionScore = Max(tensionScore, 0.0f);
qualityScore += tensionScore;
factors += 1.0f;

return factors > 0.0f ? qualityScore / factors : 0.0f;
}

    String GetQualityFeedback(F32 qualityScore) {
        if (qualityScore >= 0.8f) {
            return "Excellent conversation - engaging, well-balanced, and memorable.";
        }
        else if (qualityScore >= 0.6f) {
            return "Good conversation - interesting topics with reasonable depth.";
        }
        else if (qualityScore >= 0.4f) {
            return "Average conversation - functional but could be more engaging.";
        }
        else if (qualityScore >= 0.2f) {
            return "Below average conversation - limited engagement or depth.";
        }
        else {
            return "Poor conversation - very brief or problematic interaction.";
        }
    }
} // namespace Angaraka::AI::DialogueUtils