#pragma once

#include "Angaraka/AssetBundleConfig.hpp"
#include <queue>
#include <mutex>
#include <functional>
#include <memory>

namespace Angaraka::Core {

    // Forward declarations
    class Resource;

    enum class LoadStatus {
        Pending,
        Loading,
        Completed,
        Failed
    };

    struct LoadRequest {
        AssetDefinition asset;
        String bundleName;
        LoadStatus status = LoadStatus::Pending;
        Reference<Resource> loadedResource = nullptr;
        String errorMessage;

        // Callback for when loading completes (success or failure)
        std::function<void(const LoadRequest&)> onComplete;

        // Priority comparison for queue ordering
        bool operator<(const LoadRequest& other) const {
            // Lower priority values = higher priority in queue
            return asset.priority > other.asset.priority;
        }
    };

    class AssetLoadQueue {
    public:
        AssetLoadQueue();
        ~AssetLoadQueue() = default;

        // Add single asset to load queue
        void EnqueueAsset(const AssetDefinition& asset,
            const String& bundleName,
            std::function<void(const LoadRequest&)> onComplete = nullptr);

        // Add all assets from bundle to queue
        void EnqueueBundle(const AssetBundleConfig& bundle,
            std::function<void(const LoadRequest&)> onComplete = nullptr);

        // Get next highest priority asset to load
        std::optional<LoadRequest> DequeueNextAsset();

        // Mark asset as completed (called by worker threads)
        void MarkAssetCompleted(const String& assetId,
            Reference<Resource> resource = nullptr,
            const String& errorMessage = "");

        // Query methods
        size_t GetQueueSize() const;
        size_t GetLoadingCount() const;
        bool IsAssetQueued(const String& assetId) const;
        bool IsAssetLoading(const String& assetId) const;
        LoadStatus GetAssetStatus(const String& assetId) const;

        // Get all pending requests sorted by priority
        std::vector<LoadRequest> GetPendingRequests() const;

        // Clear all pending requests
        void Clear();

    private:
        mutable std::mutex m_queueMutex;      // Protects m_loadQueue
        mutable std::mutex m_statusMutex;     // Protects m_loadingAssets, m_completedAssets
        std::priority_queue<LoadRequest> m_loadQueue;

        // Track currently loading assets
        std::unordered_map<String, LoadRequest> m_loadingAssets;

        // Track completed assets (for status queries)
        std::unordered_map<String, LoadRequest> m_completedAssets;

        // Helper to move request to completed state
        void MoveToCompleted(const String& assetId, LoadStatus status,
            Reference<Resource> resource = nullptr,
            const String& errorMessage = "");
    };

}