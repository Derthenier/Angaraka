#pragma once

#include <Angaraka/Base.hpp>
#include <list>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace Angaraka::Core {

    // Forward declarations
    class Resource;

    /**
     * @brief Cache entry for LRU tracking with memory accounting
     */
    struct CacheEntry {
        String resourceId;
        Reference<Resource> resource;
        size_t memorySizeBytes;
        std::chrono::steady_clock::time_point lastAccessTime;
        std::chrono::steady_clock::time_point loadTime;

        CacheEntry(const String& id, Reference<Resource> res, size_t size)
            : resourceId(id)
            , resource(std::move(res))
            , memorySizeBytes(size)
            , lastAccessTime(std::chrono::steady_clock::now())
            , loadTime(std::chrono::steady_clock::now())
        {
        }
    };

    /**
     * @brief LRU iterator type for efficient list manipulation
     */
    using LRUIterator = std::list<CacheEntry>::iterator;

    /**
     * @brief Memory budget configuration
     */
    struct MemoryBudget {
        size_t maxTotalMemory = 512 * 1024 * 1024;    // 512 MB default
        size_t maxSingleResource = 64 * 1024 * 1024;   // 64 MB per resource
        size_t evictionThreshold = 90;                  // Percentage to trigger eviction
        bool enableEviction = true;
        bool logEvictions = true;

        // Helper methods
        size_t GetEvictionTriggerSize() const {
            return (maxTotalMemory * evictionThreshold) / 100;
        }
    };

    /**
     * @brief Eviction statistics for monitoring
     */
    struct EvictionStats {
        size_t totalEvictions = 0;
        size_t memoryReclaimed = 0;
        size_t lastEvictionCount = 0;
        std::chrono::steady_clock::time_point lastEvictionTime;

        void RecordEviction(size_t memoryFreed) {
            totalEvictions++;
            memoryReclaimed += memoryFreed;
            lastEvictionCount++;
            lastEvictionTime = std::chrono::steady_clock::now();
        }

        void ResetSession() {
            lastEvictionCount = 0;
        }
    };

    /**
     * @brief LRU cache with configurable memory management
     */
    class ResourceCache {
    public:
        explicit ResourceCache(const MemoryBudget& budget = MemoryBudget{});
        ~ResourceCache();

        // Cache operations
        Reference<Resource> Get(const String& resourceId);
        void Put(const String& resourceId, Reference<Resource> resource, size_t memorySize);
        void Remove(const String& resourceId);
        void Clear();

        // Memory management
        void SetMemoryBudget(const MemoryBudget& budget);
        const MemoryBudget& GetMemoryBudget() const { return m_budget; }

        // Statistics
        size_t GetCurrentMemoryUsage() const { return m_currentMemoryUsage; }
        size_t GetResourceCount() const { return m_resourceMap.size(); }
        const EvictionStats& GetEvictionStats() const { return m_stats; }
        F32 GetMemoryUtilization() const {
            return static_cast<F32>(m_currentMemoryUsage) / static_cast<F32>(m_budget.maxTotalMemory);
        }

        // Cache health check
        bool IsMemoryHealthy() const {
            return m_currentMemoryUsage < m_budget.GetEvictionTriggerSize();
        }

    private:
        // Core data structures
        std::list<CacheEntry> m_lruList;
        std::unordered_map<String, LRUIterator> m_resourceMap;

        // Memory tracking
        MemoryBudget m_budget;
        size_t m_currentMemoryUsage;
        EvictionStats m_stats;

        // Thread safety
        mutable std::mutex m_cacheMutex;
        std::atomic<bool> m_isShuttingDown{ false };

        // Internal operations
        void TouchResource(std::list<CacheEntry>::iterator it);
        void EvictIfNecessary(size_t incomingResourceSize);
        void EvictOldestResource();
        size_t EstimateResourceMemorySize(const Reference<Resource>& resource) const;

        // Validation
        bool ValidateResourceSize(size_t resourceSize) const;
    };

} // namespace Angaraka::Core