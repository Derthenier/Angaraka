#pragma once

#include "Angaraka/AIBase.hpp"
#include <nlohmann/json.hpp>

namespace Angaraka::AI {

    // Tokenizer for decoding model outputs to text
    class Tokenizer {
    public:
        Tokenizer() = default;
        ~Tokenizer() = default;

        // Load tokenizer files
        bool LoadVocabulary(const std::string& vocabFilePath);
        bool LoadSpecialTokens(const std::string& specialTokensFilePath);
        bool LoadTokenizer(const std::string& vocabFilePath, const std::string& specialTokensFilePath);

        // Text decoding
        std::string DecodeTokens(const std::vector<int64_t>& tokenIds);
        std::string DecodeToken(int64_t tokenId);

        // Token information
        bool IsSpecialToken(int64_t tokenId) const;
        bool IsEndOfTextToken(int64_t tokenId) const;
        std::string GetSpecialTokenName(int64_t tokenId) const;

        // Validation
        bool IsLoaded() const { return m_vocabularyLoaded && m_specialTokensLoaded; }
        size_t GetVocabularySize() const { return m_idToToken.size(); }

        // Cleanup and formatting
        std::string CleanDecodedText(const std::string& rawText) const;

    private:
        // Vocabulary mapping
        std::unordered_map<int64_t, std::string> m_idToToken;
        std::unordered_map<std::string, int64_t> m_tokenToId;

        // Special tokens
        std::unordered_map<int64_t, std::string> m_specialTokens;
        std::unordered_map<std::string, int64_t> m_specialTokenNames;

        // Token information
        int64_t m_eosTokenId = -1;        // End of sequence token
        int64_t m_bosTokenId = -1;        // Beginning of sequence token
        int64_t m_padTokenId = -1;        // Padding token
        int64_t m_unkTokenId = -1;        // Unknown token

        // State
        bool m_vocabularyLoaded = false;
        bool m_specialTokensLoaded = false;

        // Helper methods
        bool ParseVocabularyJson(const nlohmann::json& vocabJson);
        bool ParseSpecialTokensJson(const nlohmann::json& specialJson);
        std::string HandleSpecialCharacters(const std::string& tokenText) const;
        std::string ProcessBPEToken(const std::string& token) const;
    };

    // Tokenizer factory for creating faction-specific tokenizers
    class TokenizerFactory {
    public:
        static std::shared_ptr<Tokenizer> CreateTokenizer(const std::string& tokenizerPath);
        static std::shared_ptr<Tokenizer> CreateFactionTokenizer(const std::string& factionId, const std::string& basePath);

        // Cache tokenizers to avoid reloading
        static std::shared_ptr<Tokenizer> GetCachedTokenizer(const std::string& tokenizerKey);
        static void ClearCache();

    private:
        static std::unordered_map<std::string, std::shared_ptr<Tokenizer>> s_tokenizerCache;
    };

} // namespace Angaraka::AI