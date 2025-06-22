module;

#include "Angaraka/Base.hpp"
#include "Angaraka/ResourceCache.hpp"
#include "Angaraka/AssetBundleConfig.hpp"

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
            const String& basePath,
            Angaraka::Events::EventManager& eventBus,
            const MemoryBudget& cacheConfig = MemoryBudget{}
        );
        ~CachedResourceManager();

        // Dependency injection
        inline void SetGraphicsFactory(Reference<Core::GraphicsResourceFactory> factory) {
            std::lock_guard<std::mutex> lock(m_managerMutex);
            m_graphicsFactory = factory;
            AGK_INFO("CachedResourceManager: Graphics factory injected");
        }

        // Resource access with caching
        template<typename T>
        Reference<T> GetResource(const String& id, const String& path = {}, void* context = nullptr) {
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

            // For cache miss, use factory instead of direct construction:
            Reference<Resource> newResource;
            if (m_graphicsFactory) {
                // Determine asset type and use appropriate factory method
                AssetDefinition tempAsset{};
                tempAsset.id = id;
                tempAsset.path = m_basePath + "/" + path;
                // You'll need logic to determine asset type from ID/extension

                newResource = m_graphicsFactory->Create(tempAsset, context);
                
                if (newResource) {
                    m_cache.Put(id, newResource, EstimateResourceSize(newResource));
                }

                return newResource;
            }

            AGK_ERROR("CachedResourceManager: Failed to load resource '{}'", id);
            return nullptr;
        }

        // Cache management
        void UnloadResource(const String& id);
        void UnloadAllResources();
        void SetCacheConfig(const MemoryBudget& config);

        // Statistics and monitoring
        const EvictionStats& GetCacheStats() const { return m_cache.GetEvictionStats(); }
        size_t GetCacheMemoryUsage() const { return m_cache.GetCurrentMemoryUsage(); }
        F32 GetCacheUtilization() const { return m_cache.GetMemoryUtilization(); }
        bool IsCacheHealthy() const { return m_cache.IsMemoryHealthy(); }

        // Maintenance operations
        void TrimCache(); // Manual eviction trigger
        void LogCacheStatus() const;

    private:
        String m_basePath;
        ResourceCache m_cache;
        Angaraka::Events::EventManager& m_eventBus;
        mutable std::mutex m_managerMutex;

        Reference<Core::GraphicsResourceFactory> m_graphicsFactory;

        // Generic fallback estimation
        size_t EstimateResourceSize(const Reference<Resource>& resource) const;
    };
} // namespace Angaraka::Core