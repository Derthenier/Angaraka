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
        std::string bundleName;
        LoadStatus status = LoadStatus::Pending;
        std::shared_ptr<Resource> loadedResource = nullptr;
        std::string errorMessage;

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
            const std::string& bundleName,
            std::function<void(const LoadRequest&)> onComplete = nullptr);

        // Add all assets from bundle to queue
        void EnqueueBundle(const AssetBundleConfig& bundle,
            std::function<void(const LoadRequest&)> onComplete = nullptr);

        // Get next highest priority asset to load
        std::optional<LoadRequest> DequeueNextAsset();

        // Mark asset as completed (called by worker threads)
        void MarkAssetCompleted(const std::string& assetId,
            std::shared_ptr<Resource> resource = nullptr,
            const std::string& errorMessage = "");

        // Query methods
        size_t GetQueueSize() const;
        size_t GetLoadingCount() const;
        bool IsAssetQueued(const std::string& assetId) const;
        bool IsAssetLoading(const std::string& assetId) const;
        LoadStatus GetAssetStatus(const std::string& assetId) const;

        // Get all pending requests sorted by priority
        std::vector<LoadRequest> GetPendingRequests() const;

        // Clear all pending requests
        void Clear();

    private:
        mutable std::mutex m_queueMutex;      // Protects m_loadQueue
        mutable std::mutex m_statusMutex;     // Protects m_loadingAssets, m_completedAssets
        std::priority_queue<LoadRequest> m_loadQueue;

        // Track currently loading assets
        std::unordered_map<std::string, LoadRequest> m_loadingAssets;

        // Track completed assets (for status queries)
        std::unordered_map<std::string, LoadRequest> m_completedAssets;

        // Helper to move request to completed state
        void MoveToCompleted(const std::string& assetId, LoadStatus status,
            std::shared_ptr<Resource> resource = nullptr,
            const std::string& errorMessage = "");
    };

}