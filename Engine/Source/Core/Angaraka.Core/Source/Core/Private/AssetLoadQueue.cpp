#include "Angaraka/Asset/LoadQueue.hpp"
#include "Angaraka/Log.hpp"

namespace Angaraka::Core {

    AssetLoadQueue::AssetLoadQueue() {
        AGK_INFO("AssetLoadQueue: Initialized");
    }

    void AssetLoadQueue::EnqueueAsset(const AssetDefinition& asset,
        const String& bundleName,
        std::function<void(const LoadRequest&)> onComplete) {
        // Check status first (separate locks)
        if (IsAssetQueued(asset.id) || IsAssetLoading(asset.id)) {
            AGK_WARN("Asset {} already in queue or loading", asset.id);
            return;
        }

        std::lock_guard<std::mutex> lock(m_queueMutex);

        LoadRequest request;
        request.asset = asset;
        request.bundleName = bundleName;
        request.onComplete = onComplete;

        m_loadQueue.push(request);
        AGK_TRACE("Enqueued asset: {} (priority: {})", asset.id, asset.priority);
    }

    void AssetLoadQueue::EnqueueBundle(const AssetBundleConfig& bundle,
        std::function<void(const LoadRequest&)> onComplete) {
        auto sortedAssets = bundle.GetAssetsByPriority();

        for (const auto& asset : sortedAssets) {
            EnqueueAsset(asset, bundle.name, onComplete);
        }

        AGK_INFO("Enqueued {} assets from bundle: {}", sortedAssets.size(), bundle.name);
    }

    std::optional<LoadRequest> AssetLoadQueue::DequeueNextAsset() {
        std::lock_guard<std::mutex> queueLock(m_queueMutex);
        std::lock_guard<std::mutex> statusLock(m_statusMutex);

        if (m_loadQueue.empty()) {
            return std::nullopt;
        }

        LoadRequest request = m_loadQueue.top();
        m_loadQueue.pop();

        request.status = LoadStatus::Loading;
        m_loadingAssets[request.asset.id] = request;

        AGK_TRACE("Dequeued asset for loading: {}", request.asset.id);
        return request;
    }

    void AssetLoadQueue::MarkAssetCompleted(const String& assetId,
        Reference<Resource> resource,
        const String& errorMessage) {
        std::lock_guard<std::mutex> lock(m_queueMutex);

        auto it = m_loadingAssets.find(assetId);
        if (it == m_loadingAssets.end()) {
            AGK_WARN("Attempted to mark unknown asset as completed: {}", assetId);
            return;
        }

        LoadStatus status = resource ? LoadStatus::Completed : LoadStatus::Failed;
        MoveToCompleted(assetId, status, resource, errorMessage);
    }

    size_t AssetLoadQueue::GetQueueSize() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_loadQueue.size();
    }

    size_t AssetLoadQueue::GetLoadingCount() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_loadingAssets.size();
    }

    bool AssetLoadQueue::IsAssetQueued(const String& assetId) const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        auto queueCopy = m_loadQueue;

        while (!queueCopy.empty()) {
            if (queueCopy.top().asset.id == assetId) {
                return true;
            }
            queueCopy.pop();
        }
        return false;
    }

    bool AssetLoadQueue::IsAssetLoading(const String& assetId) const {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        return m_loadingAssets.find(assetId) != m_loadingAssets.end();
    }

    LoadStatus AssetLoadQueue::GetAssetStatus(const String& assetId) const {
        std::lock_guard<std::mutex> lock(m_statusMutex);

        if (m_loadingAssets.find(assetId) != m_loadingAssets.end()) {
            return LoadStatus::Loading;
        }

        auto it = m_completedAssets.find(assetId);
        if (it != m_completedAssets.end()) {
            return it->second.status;
        }

        return LoadStatus::Pending;
    }

    std::vector<LoadRequest> AssetLoadQueue::GetPendingRequests() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);

        std::vector<LoadRequest> requests;
        auto queueCopy = m_loadQueue;

        while (!queueCopy.empty()) {
            requests.push_back(queueCopy.top());
            queueCopy.pop();
        }

        return requests;
    }

    void AssetLoadQueue::Clear() {
        std::lock_guard<std::mutex> lock(m_queueMutex);

        // Clear queue
        while (!m_loadQueue.empty()) {
            m_loadQueue.pop();
        }

        // Clear loading and completed maps
        m_loadingAssets.clear();
        m_completedAssets.clear();

        AGK_INFO("AssetLoadQueue cleared");
    }

    void AssetLoadQueue::MoveToCompleted(const String& assetId, LoadStatus status,
        Reference<Resource> resource,
        const String& errorMessage) {
        auto it = m_loadingAssets.find(assetId);
        if (it == m_loadingAssets.end()) return;

        LoadRequest request = it->second;
        request.status = status;
        request.loadedResource = resource;
        request.errorMessage = errorMessage;

        // Move to completed
        m_completedAssets[assetId] = request;
        m_loadingAssets.erase(it);

        // Call completion callback if provided
        if (request.onComplete) {
            request.onComplete(request);
        }

        AGK_TRACE("Asset completed: {} (status: {})", assetId, static_cast<int>(status));
    }

}