#pragma once

#include "Angaraka/AssetBundleConfig.hpp"
#include "Angaraka/AssetBundleLoader.hpp"
#include "Angaraka/AssetLoadQueue.hpp"
#include "Angaraka/AssetWorkerPool.hpp"
#include <memory>
#include <unordered_map>
#include <functional>

import Angaraka.Core.ResourceCache;

namespace Angaraka::Core {

    enum class BundleLoadState {
        NotLoaded,
        Loading,
        Loaded,
        Failed
    };

    struct BundleLoadProgress {
        std::string bundleName;
        BundleLoadState state = BundleLoadState::NotLoaded;
        float progress = 0.0f;  // 0.0 to 1.0
        size_t assetsLoaded = 0;
        size_t totalAssets = 0;
        std::string errorMessage;
    };

    using BundleProgressCallback = std::function<void(const BundleLoadProgress&)>;

    class BundleManager {
    public:
        explicit BundleManager(CachedResourceManager* resourceManager, void* context = nullptr);
        ~BundleManager();

        // Initialize with bundles directory
        bool Initialize(const std::filesystem::path& bundlesDirectory);
        void Shutdown();

        // Bundle operations
        bool LoadBundle(const std::string& bundleName, BundleProgressCallback callback = nullptr);
        bool UnloadBundle(const std::string& bundleName);
        void LoadAllAutoLoadBundles();

        // Async operations
        void StartAsyncLoading();
        void StopAsyncLoading();
        void PauseAsyncLoading();
        void ResumeAsyncLoading();

        // Query methods
        bool IsBundleLoaded(const std::string& bundleName) const;
        BundleLoadState GetBundleState(const std::string& bundleName) const;
        BundleLoadProgress GetBundleProgress(const std::string& bundleName) const;
        std::vector<std::string> GetLoadedBundles() const;
        std::vector<std::string> GetAvailableBundles() const;

        // Asset access
        template <typename T>
        std::shared_ptr<T> GetAsset(const std::string& assetId, void* context = nullptr);
        bool IsAssetLoaded(const std::string& assetId) const;

        // Memory management
        void UnloadUnusedAssets();
        size_t GetMemoryUsage() const;

        // Progress tracking
        void SetGlobalProgressCallback(BundleProgressCallback callback);

    private:
        void RegisterAssetLoaders();
        bool ValidateAndResolveDependencies(const std::string& bundleName,
            std::vector<std::string>& loadOrder);
        void OnAssetLoadComplete(const LoadRequest& request);
        void UpdateBundleProgress(const std::string& bundleName);

        bool LoadBundleInternal(const std::string& bundleName, BundleProgressCallback callback);

        CachedResourceManager* m_resourceManager;
        std::unique_ptr<BundleLoader> m_bundleLoader;
        std::shared_ptr<AssetLoadQueue> m_loadQueue;
        std::unique_ptr<AssetWorkerPool> m_workerPool;

        // Bundle tracking
        std::unordered_map<std::string, AssetBundleConfig> m_availableBundles;
        std::unordered_map<std::string, BundleLoadState> m_bundleStates;
        std::unordered_map<std::string, BundleProgressCallback> m_bundleCallbacks;

        // Asset ID to bundle mapping
        std::unordered_map<std::string, std::string> m_assetToBundleMap;

        void* m_context;

        BundleProgressCallback m_globalProgressCallback;
        mutable std::mutex m_bundlesMutex;
    };

    template <typename T>
    std::shared_ptr<T> BundleManager::GetAsset(const std::string& assetId, void* context) {
        return m_resourceManager->GetResource(assetId, context);
    }
}