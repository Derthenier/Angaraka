module;

#include "Angaraka/Base.hpp"

module Angaraka.Core.ResourceCache;

import Angaraka.Core.Resources;

namespace Angaraka::Core {

    ResourceCache::ResourceCache(const MemoryBudget& budget)
        : m_budget(budget)
        , m_currentMemoryUsage(0)
        , m_stats{}
    {
        AGK_INFO("ResourceCache: Initialized with {}MB budget, {}% eviction threshold",
            m_budget.maxTotalMemory / (1024 * 1024), m_budget.evictionThreshold);
    }

    ResourceCache::~ResourceCache() {
        AGK_INFO("ResourceCache: Destroyed. Final stats - {} resources, {}MB used, {} total evictions",
            m_resourceMap.size(), m_currentMemoryUsage / (1024 * 1024), m_stats.totalEvictions);
        Clear();
    }

    std::shared_ptr<Resource> ResourceCache::Get(const std::string& resourceId) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);

        auto mapIt = m_resourceMap.find(resourceId);
        if (mapIt == m_resourceMap.end()) {
            return nullptr; // Cache miss
        }

        // Move to front (most recently used)
        TouchResource(mapIt->second);

        AGK_TRACE("ResourceCache: Cache hit for '{}'", resourceId);
        return mapIt->second->resource;
    }

    void ResourceCache::Put(const std::string& resourceId, std::shared_ptr<Resource> resource, size_t memorySize) {
        if (!resource) {
            AGK_WARN("ResourceCache: Attempted to cache null resource '{}'", resourceId);
            return;
        }

        if (!ValidateResourceSize(memorySize)) {
            AGK_ERROR("ResourceCache: Resource '{}' exceeds maximum size limit ({}MB > {}MB)",
                resourceId, memorySize / (1024 * 1024), m_budget.maxSingleResource / (1024 * 1024));
            return;
        }

        std::lock_guard<std::mutex> lock(m_cacheMutex);

        // Check if resource already exists
        auto existingIt = m_resourceMap.find(resourceId);
        if (existingIt != m_resourceMap.end()) {
            // Update existing entry
            size_t oldSize = existingIt->second->memorySizeBytes;
            existingIt->second->resource = resource;
            existingIt->second->memorySizeBytes = memorySize;
            existingIt->second->lastAccessTime = std::chrono::steady_clock::now();

            m_currentMemoryUsage = m_currentMemoryUsage - oldSize + memorySize;
            TouchResource(existingIt->second);

            AGK_DEBUG("ResourceCache: Updated existing resource '{}' ({}MB -> {}MB)",
                resourceId, oldSize / (1024 * 1024), memorySize / (1024 * 1024));
        }
        else {
            // Evict if necessary before adding new resource
            EvictIfNecessary(memorySize);

            // Add new entry at front of LRU list
            m_lruList.emplace_front(resourceId, resource, memorySize);
            m_resourceMap[resourceId] = m_lruList.begin();
            m_currentMemoryUsage += memorySize;

            AGK_DEBUG("ResourceCache: Cached new resource '{}' ({}MB). Total usage: {}MB",
                resourceId, memorySize / (1024 * 1024), m_currentMemoryUsage / (1024 * 1024));
        }
    }

    void ResourceCache::Remove(const std::string& resourceId) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);

        auto mapIt = m_resourceMap.find(resourceId);
        if (mapIt != m_resourceMap.end()) {
            size_t memoryFreed = mapIt->second->memorySizeBytes;
            m_lruList.erase(mapIt->second);
            m_resourceMap.erase(mapIt);
            m_currentMemoryUsage -= memoryFreed;

            AGK_DEBUG("ResourceCache: Manually removed '{}' ({}MB freed)",
                resourceId, memoryFreed / (1024 * 1024));
        }
    }

    void ResourceCache::Clear() {
        m_isShuttingDown = true;
        // Don't lock if already shutting down
        if (!m_isShuttingDown) {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
        }

        size_t resourceCount = m_resourceMap.size();
        size_t memoryFreed = m_currentMemoryUsage;

        m_lruList.clear();
        m_resourceMap.clear();
        m_currentMemoryUsage = 0;

        AGK_INFO("ResourceCache: Cleared {} resources, freed {}MB",
            resourceCount, memoryFreed / (1024 * 1024));
    }

    void ResourceCache::SetMemoryBudget(const MemoryBudget& budget) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);

        AGK_INFO("ResourceCache: Updating budget from {}MB to {}MB",
            m_budget.maxTotalMemory / (1024 * 1024), budget.maxTotalMemory / (1024 * 1024));

        m_budget = budget;

        // Trigger eviction if current usage exceeds new budget
        if (m_currentMemoryUsage > m_budget.GetEvictionTriggerSize()) {
            EvictIfNecessary(0);
        }
    }

    void ResourceCache::TouchResource(std::list<CacheEntry>::iterator it) {
        // Move to front of list (most recently used)
        it->lastAccessTime = std::chrono::steady_clock::now();
        m_lruList.splice(m_lruList.begin(), m_lruList, it);
    }

    void ResourceCache::EvictIfNecessary(size_t incomingResourceSize) {
        if (!m_budget.enableEviction) {
            return;
        }

        size_t targetMemoryUsage = m_currentMemoryUsage + incomingResourceSize;
        size_t evictionTrigger = m_budget.GetEvictionTriggerSize();

        if (targetMemoryUsage <= evictionTrigger) {
            return; // No eviction needed
        }

        // Calculate how much memory we need to free
        size_t memoryToFree = targetMemoryUsage - evictionTrigger;
        size_t memoryFreed = 0;
        size_t resourcesEvicted = 0;

        if (m_budget.logEvictions) {
            AGK_INFO("ResourceCache: Starting eviction - need to free {}MB (current: {}MB + incoming: {}MB > trigger: {}MB)",
                memoryToFree / (1024 * 1024), m_currentMemoryUsage / (1024 * 1024),
                incomingResourceSize / (1024 * 1024), evictionTrigger / (1024 * 1024));
        }

        // Evict from least recently used (back of list)
        while (memoryFreed < memoryToFree && !m_lruList.empty()) {
            auto& oldestEntry = m_lruList.back();

            // Check if resource is still referenced elsewhere
            if (oldestEntry.resource.use_count() <= 1) {
                size_t entrySize = oldestEntry.memorySizeBytes;
                std::string entryId = oldestEntry.resourceId;

                // Remove from both structures
                m_resourceMap.erase(entryId);
                m_lruList.pop_back();

                memoryFreed += entrySize;
                resourcesEvicted++;
                m_currentMemoryUsage -= entrySize;
                m_stats.RecordEviction(entrySize);

                if (m_budget.logEvictions) {
                    AGK_DEBUG("ResourceCache: Evicted '{}' ({}MB freed)", entryId, entrySize / (1024 * 1024));
                }
            }
            else {
                // Resource still in use, move to front and try next
                TouchResource(std::prev(m_lruList.end()));
                break;
            }
        }

        if (m_budget.logEvictions && resourcesEvicted > 0) {
            AGK_INFO("ResourceCache: Eviction complete - {} resources evicted, {}MB freed",
                resourcesEvicted, memoryFreed / (1024 * 1024));
        }
    }

    void ResourceCache::EvictOldestResource() {
        if (m_lruList.empty()) {
            return;
        }

        auto& oldestEntry = m_lruList.back();
        size_t memoryFreed = oldestEntry.memorySizeBytes;
        std::string resourceId = oldestEntry.resourceId;

        m_resourceMap.erase(resourceId);
        m_lruList.pop_back();
        m_currentMemoryUsage -= memoryFreed;
        m_stats.RecordEviction(memoryFreed);

        AGK_DEBUG("ResourceCache: Force evicted oldest resource '{}' ({}MB)",
            resourceId, memoryFreed / (1024 * 1024));
    }

    size_t ResourceCache::EstimateResourceMemorySize(const std::shared_ptr<Resource>& resource) const {
        // Fallback estimation if size not provided
        // This is a conservative estimate - derived classes should override for accuracy
        return 1024 * 1024; // 1MB default estimate
    }

    bool ResourceCache::ValidateResourceSize(size_t resourceSize) const {
        return resourceSize <= m_budget.maxSingleResource;
    }

} // namespace Angaraka::Core