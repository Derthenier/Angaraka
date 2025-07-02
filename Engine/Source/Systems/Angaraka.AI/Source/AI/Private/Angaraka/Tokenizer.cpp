// Engine/Source/Systems/Angaraka.AI/Source/AI/Modules/Tokenizer.cpp
#include "Angaraka/Tokenizer.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <nlohmann/json.hpp>

namespace Angaraka::AI {

    bool Tokenizer::LoadVocabulary(const String& vocabFilePath) {
        AGK_INFO("Tokenizer: Loading vocabulary from '{0}'", vocabFilePath);

        if (!std::filesystem::exists(vocabFilePath)) {
            AGK_ERROR("Tokenizer: Vocabulary file not found: '{0}'", vocabFilePath);
            return false;
        }

        try {
            std::ifstream file(vocabFilePath);
            nlohmann::json vocabJson;
            file >> vocabJson;

            if (!ParseVocabularyJson(vocabJson)) {
                AGK_ERROR("Tokenizer: Failed to parse vocabulary JSON");
                return false;
            }

            m_vocabularyLoaded = true;
            AGK_INFO("Tokenizer: Successfully loaded {0} vocabulary entries", m_idToToken.size());
            return true;

        }
        catch (const std::exception& e) {
            AGK_ERROR("Tokenizer: Exception loading vocabulary: {0}", e.what());
            return false;
        }
    }

    bool Tokenizer::LoadSpecialTokens(const String& specialTokensFilePath) {
        AGK_INFO("Tokenizer: Loading special tokens from '{0}'", specialTokensFilePath);

        if (!std::filesystem::exists(specialTokensFilePath)) {
            AGK_ERROR("Tokenizer: Special tokens file not found: '{0}'", specialTokensFilePath);
            return false;
        }

        try {
            std::ifstream file(specialTokensFilePath);
            nlohmann::json specialJson;
            file >> specialJson;

            if (!ParseSpecialTokensJson(specialJson)) {
                AGK_ERROR("Tokenizer: Failed to parse special tokens JSON");
                return false;
            }

            m_specialTokensLoaded = true;
            AGK_INFO("Tokenizer: Successfully loaded {0} special tokens", m_specialTokens.size());
            return true;

        }
        catch (const std::exception& e) {
            AGK_ERROR("Tokenizer: Exception loading special tokens: {0}", e.what());
            return false;
        }
    }

    bool Tokenizer::LoadTokenizer(const String& vocabFilePath, const String& specialTokensFilePath) {
        bool vocabLoaded = LoadVocabulary(vocabFilePath);
        bool specialLoaded = LoadSpecialTokens(specialTokensFilePath);

        return vocabLoaded && specialLoaded;
    }

    String Tokenizer::DecodeTokens(const std::vector<I64>& tokenIds) {
        if (!IsLoaded()) {
            AGK_ERROR("Tokenizer: Cannot decode tokens - tokenizer not loaded");
            return "";
        }

        if (tokenIds.empty()) {
            return "";
        }

        String result;
        result.reserve(tokenIds.size() * 4); // Rough estimate

        for (I64 tokenId : tokenIds) {
            // Skip special tokens that shouldn't appear in output
            if (tokenId == m_eosTokenId || tokenId == m_bosTokenId || tokenId == m_padTokenId) {
                continue;
            }

            String tokenText = DecodeToken(tokenId);
            if (!tokenText.empty()) {
                result += tokenText;
            }
        }

        // Clean up the decoded text
        return CleanDecodedText(result);
    }

    String Tokenizer::DecodeToken(I64 tokenId) {
        // Check special tokens first
        auto specialIt = m_specialTokens.find(tokenId);
        if (specialIt != m_specialTokens.end()) {
            return specialIt->second;
        }

        // Check regular vocabulary
        auto vocabIt = m_idToToken.find(tokenId);
        if (vocabIt != m_idToToken.end()) {
            return ProcessBPEToken(vocabIt->second);
        }

        // Unknown token
        AGK_WARN("Tokenizer: Unknown token ID: {0}", tokenId);
        return "<unk>";
    }

    bool Tokenizer::IsSpecialToken(I64 tokenId) const {
        return m_specialTokens.find(tokenId) != m_specialTokens.end();
    }

    bool Tokenizer::IsEndOfTextToken(I64 tokenId) const {
        return tokenId == m_eosTokenId;
    }

    String Tokenizer::GetSpecialTokenName(I64 tokenId) const {
        auto it = m_specialTokens.find(tokenId);
        return it != m_specialTokens.end() ? it->second : "";
    }

    String Tokenizer::CleanDecodedText(const String& rawText) const {
        String cleaned = rawText;

        // Handle BPE merge artifacts (Ġ represents space in some tokenizers)
        std::regex bpeSpace(R"(Ġ)");
        cleaned = std::regex_replace(cleaned, bpeSpace, " ");

        // Handle other BPE artifacts
        std::regex bpeNewline(R"(Ċ)");
        cleaned = std::regex_replace(cleaned, bpeNewline, "\n");

        // Remove multiple consecutive spaces
        std::regex multipleSpaces(R"( {2,})");
        cleaned = std::regex_replace(cleaned, multipleSpaces, " ");

        // Remove multiple consecutive newlines
        std::regex multipleNewlines(R"(\n{3,})");
        cleaned = std::regex_replace(cleaned, multipleNewlines, "\n\n");

        // Trim leading/trailing whitespace
        cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
        cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);

        return cleaned;
    }

    bool Tokenizer::ParseVocabularyJson(const nlohmann::json& vocabJson) {
        try {
            m_idToToken.clear();
            m_tokenToId.clear();

            // Vocabulary JSON format can vary, handle common formats
            if (vocabJson.is_object()) {
                // Format: {"token": id, ...}
                for (auto& [token, id] : vocabJson.items()) {
                    if (id.is_number_integer()) {
                        I64 tokenId = id.get<I64>();
                        m_idToToken[tokenId] = token;
                        m_tokenToId[token] = tokenId;
                    }
                }
            }
            else if (vocabJson.is_array()) {
                // Format: ["token0", "token1", ...]
                for (size_t i = 0; i < vocabJson.size(); ++i) {
                    if (vocabJson[i].is_string()) {
                        String token = vocabJson[i].get<String>();
                        m_idToToken[static_cast<I64>(i)] = token;
                        m_tokenToId[token] = static_cast<I64>(i);
                    }
                }
            }
            else {
                AGK_ERROR("Tokenizer: Unsupported vocabulary JSON format");
                return false;
            }

            AGK_INFO("Tokenizer: Parsed {0} vocabulary entries", m_idToToken.size());
            return true;

        }
        catch (const std::exception& e) {
            AGK_ERROR("Tokenizer: Exception parsing vocabulary: {0}", e.what());
            return false;
        }
    }

    bool Tokenizer::ParseSpecialTokensJson(const nlohmann::json& specialJson) {
        try {
            m_specialTokens.clear();
            m_specialTokenNames.clear();

            // Reset special token IDs
            m_eosTokenId = -1;
            m_bosTokenId = -1;
            m_padTokenId = -1;
            m_unkTokenId = -1;

            // Parse special tokens
            for (auto& [key, value] : specialJson.items()) {
                if (value.is_object() && value.contains("content") && value.contains("id")) {
                    String content = value["content"].get<String>();
                    I64 id = value["id"].get<I64>();

                    m_specialTokens[id] = content;
                    m_specialTokenNames[content] = id;

                    // Identify common special tokens
                    if (key == "eos_token" || content == "<|endoftext|>" || content == "</s>") {
                        m_eosTokenId = id;
                        AGK_INFO("Tokenizer: EOS token: '{0}' (ID: {1})", content, id);
                    }
                    else if (key == "bos_token" || content == "<|startoftext|>" || content == "<s>") {
                        m_bosTokenId = id;
                        AGK_INFO("Tokenizer: BOS token: '{0}' (ID: {1})", content, id);
                    }
                    else if (key == "pad_token" || content == "<|pad|>") {
                        m_padTokenId = id;
                        AGK_INFO("Tokenizer: PAD token: '{0}' (ID: {1})", content, id);
                    }
                    else if (key == "unk_token" || content == "<|unk|>") {
                        m_unkTokenId = id;
                        AGK_INFO("Tokenizer: UNK token: '{0}' (ID: {1})", content, id);
                    }
                }
                else if (value.is_number_integer()) {
                    // Simple format: {"<special>": id}
                    I64 id = value.get<I64>();
                    m_specialTokens[id] = key;
                    m_specialTokenNames[key] = id;
                }
            }

            AGK_INFO("Tokenizer: Parsed {0} special tokens", m_specialTokens.size());
            return true;

        }
        catch (const std::exception& e) {
            AGK_ERROR("Tokenizer: Exception parsing special tokens: {0}", e.what());
            return false;
        }
    }

    String Tokenizer::ProcessBPEToken(const String& token) const {
        String processed = token;

        // Handle common BPE encoding artifacts
        if (processed.substr(0, 2) == "Ġ") {
            // GPT-style: Ġ prefix indicates the token starts a new word
            processed = " " + processed.substr(2);
        }
        else if (processed.substr(0, 2) == "##") {
            // BERT-style: ## prefix indicates token continuation
            processed = processed.substr(2);
        }

        return processed;
    }

    // ===== TokenizerFactory Implementation =====
    std::unordered_map<String, std::shared_ptr<Tokenizer>> TokenizerFactory::s_tokenizerCache;

    std::shared_ptr<Tokenizer> TokenizerFactory::CreateTokenizer(const String& tokenizerPath) {
        String vocabPath = tokenizerPath + "/tokenizer_vocab.json";
        String specialPath = tokenizerPath + "/special_tokens.json";

        auto tokenizer = std::make_shared<Tokenizer>();
        if (tokenizer->LoadTokenizer(vocabPath, specialPath)) {
            return tokenizer;
        }

        AGK_ERROR("TokenizerFactory: Failed to create tokenizer from '{0}'", tokenizerPath);
        return nullptr;
    }

    std::shared_ptr<Tokenizer> TokenizerFactory::CreateFactionTokenizer(const String& factionId, const String& basePath) {
        String tokenizerPath = basePath + "/" + factionId;
        String cacheKey = factionId + "_tokenizer";

        // Check cache first
        auto cached = GetCachedTokenizer(cacheKey);
        if (cached) {
            return cached;
        }

        // Create new tokenizer
        auto tokenizer = CreateTokenizer(tokenizerPath);
        if (tokenizer) {
            s_tokenizerCache[cacheKey] = tokenizer;
        }

        return tokenizer;
    }

    std::shared_ptr<Tokenizer> TokenizerFactory::GetCachedTokenizer(const String& tokenizerKey) {
        auto it = s_tokenizerCache.find(tokenizerKey);
        return it != s_tokenizerCache.end() ? it->second : nullptr;
    }

    void TokenizerFactory::ClearCache() {
        s_tokenizerCache.clear();
        AGK_INFO("TokenizerFactory: Cleared tokenizer cache");
    }

} // namespace Angaraka::AI