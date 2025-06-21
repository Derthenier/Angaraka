module;

#include "Angaraka/Base.hpp"
#include "Angaraka/ResourceCache.hpp"

export module Angaraka.Core.ResourceCache;

export import Angaraka.Core.Resources;
export import Angaraka.Core.Events;

import Angaraka.Core.GraphicsFactory;

namespace Angaraka::Core {

    // Re-export cache types for public use
    export struct CacheEntry;
    export struct MemoryBudget;
    export struct EvictionStats;
    export class ResourceCache;

    /**
     * @brief Enhanced ResourceManager with LRU cache integration
     */
    export class CachedResourceManager {
    public:
        explicit CachedResourceManager(
            Angaraka::Events::EventManager& eventBus,
            const MemoryBudget& cacheConfig = MemoryBudget{}
        );
        ~CachedResourceManager();

        // Dependency injection
        inline void SetGraphicsFactory(std::shared_ptr<IGraphicsResourceFactory> factory) {
            std::lock_guard<std::mutex> lock(m_managerMutex);
            m_graphicsFactory = factory;
            AGK_INFO("CachedResourceManager: Graphics factory injected");
        }

        // Resource access with caching
        template<typename T>
        std::shared_ptr<T> GetResource(const std::string& id, void* context = nullptr) {
            std::lock_guard<std::mutex> lock(m_managerMutex);

            // Check cache first
            if (auto cached = m_cache.Get(id)) {
                if (auto typedResource = std::dynamic_pointer_cast<T>(cached)) {
                    AGK_TRACE("CachedResourceManager: Cache hit for '{}'", id);
                    return typedResource;
                }
                // Type mismatch - remove invalid entry
                m_cache.Remove(id);
                AGK_WARN("CachedResourceManager: Type mismatch for cached resource '{}', removing", id);
            }

            // Cache miss - load new resource
            AGK_DEBUG("CachedResourceManager: Cache miss for '{}', loading...", id);
            auto newResource = std::make_shared<T>(id);

            if (newResource->Load(id, context)) {
                // Calculate memory size and cache it
                size_t memorySize = EstimateResourceSize(newResource);
                m_cache.Put(id, newResource, memorySize);

                AGK_INFO("CachedResourceManager: Loaded and cached '{}' ({}MB)",
                    id, memorySize / (1024 * 1024));
                return newResource;
            }

            AGK_ERROR("CachedResourceManager: Failed to load resource '{}'", id);
            return nullptr;
        }

        // Cache management
        void UnloadResource(const std::string& id);
        void UnloadAllResources();
        void SetCacheConfig(const MemoryBudget& config);

        // Statistics and monitoring
        const EvictionStats& GetCacheStats() const { return m_cache.GetEvictionStats(); }
        size_t GetCacheMemoryUsage() const { return m_cache.GetCurrentMemoryUsage(); }
        float GetCacheUtilization() const { return m_cache.GetMemoryUtilization(); }
        bool IsCacheHealthy() const { return m_cache.IsMemoryHealthy(); }

        // Maintenance operations
        void TrimCache(); // Manual eviction trigger
        void LogCacheStatus() const;

    private:
        ResourceCache m_cache;
        Angaraka::Events::EventManager& m_eventBus;
        mutable std::mutex m_managerMutex;

        std::shared_ptr<IGraphicsResourceFactory> m_graphicsFactory;

        // Generic fallback estimation
        size_t EstimateResourceSize(const std::shared_ptr<Resource>& resource) const;
    };
} // namespace Angaraka::Core