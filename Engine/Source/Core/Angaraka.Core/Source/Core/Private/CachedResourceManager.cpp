module;

#include <Angaraka/Base.hpp>

module Angaraka.Core.ResourceCache;

namespace Angaraka::Core {

    CachedResourceManager::CachedResourceManager(
        const String& basePath,
        const MemoryBudget& cacheConfig)
        : m_basePath(basePath)
        , m_cache(cacheConfig)
    {
        AGK_INFO("CachedResourceManager: Initialized with {}MB cache budget",
            cacheConfig.maxTotalMemory / (1024 * 1024));
    }

    CachedResourceManager::~CachedResourceManager() {
        AGK_INFO("CachedResourceManager: Shutting down");
        UnloadAllResources();
    }

    void CachedResourceManager::UnloadResource(const String& id) {
        std::lock_guard<std::mutex> lock(m_managerMutex);
        m_cache.Remove(id);
        AGK_DEBUG("CachedResourceManager: Unloaded resource '{}'", id);
    }

    void CachedResourceManager::UnloadAllResources() {
        std::lock_guard<std::mutex> lock(m_managerMutex);
        m_cache.Clear();
        AGK_INFO("CachedResourceManager: Unloaded all resources");
    }

    void CachedResourceManager::SetCacheConfig(const MemoryBudget& config) {
        std::lock_guard<std::mutex> lock(m_managerMutex);
        m_cache.SetMemoryBudget(config);
        AGK_INFO("CachedResourceManager: Updated cache configuration");
    }

    void CachedResourceManager::TrimCache() {
        std::lock_guard<std::mutex> lock(m_managerMutex);

        // Force eviction by setting temporary aggressive budget
        auto currentBudget = m_cache.GetMemoryBudget();
        MemoryBudget trimBudget = currentBudget;
        trimBudget.evictionThreshold = 50; // Aggressive threshold

        m_cache.SetMemoryBudget(trimBudget);

        // Restore original budget
        m_cache.SetMemoryBudget(currentBudget);

        AGK_INFO("CachedResourceManager: Cache trimmed manually");
    }

    void CachedResourceManager::LogCacheStatus() const {
        std::lock_guard<std::mutex> lock(m_managerMutex);

        auto stats = m_cache.GetEvictionStats();
        size_t memoryUsage = m_cache.GetCurrentMemoryUsage();
        size_t resourceCount = m_cache.GetResourceCount();
        F32 utilization = m_cache.GetMemoryUtilization();

        AGK_INFO("Cache Status: {} resources, {}MB used ({:.1f}% utilization), {} total evictions",
            resourceCount,
            memoryUsage / (1024 * 1024),
            utilization * 100.0f,
            stats.totalEvictions);

        if (!m_cache.IsMemoryHealthy()) {
            AGK_WARN("Cache health warning: High memory usage detected");
        }
    }

    size_t CachedResourceManager::EstimateResourceSize(const Reference<Resource>& resource) const {
        return resource->GetSizeInBytes();
    }

} // namespace Angaraka::Core