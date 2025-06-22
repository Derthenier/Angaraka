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
        String bundleName;
        BundleLoadState state = BundleLoadState::NotLoaded;
        F32 progress = 0.0f;  // 0.0 to 1.0
        size_t assetsLoaded = 0;
        size_t totalAssets = 0;
        String errorMessage;
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
        bool LoadBundle(const String& bundleName, BundleProgressCallback callback = nullptr);
        bool UnloadBundle(const String& bundleName);
        void LoadAllAutoLoadBundles();

        // Async operations
        void StartAsyncLoading();
        void StopAsyncLoading();
        void PauseAsyncLoading();
        void ResumeAsyncLoading();

        // Query methods
        bool IsBundleLoaded(const String& bundleName) const;
        BundleLoadState GetBundleState(const String& bundleName) const;
        BundleLoadProgress GetBundleProgress(const String& bundleName) const;
        std::vector<String> GetLoadedBundles() const;
        std::vector<String> GetAvailableBundles() const;

        // Asset access
        template <typename T>
        Reference<T> GetAsset(const String& assetId, void* context = nullptr);
        bool IsAssetLoaded(const String& assetId) const;

        // Memory management
        void UnloadUnusedAssets();
        size_t GetMemoryUsage() const;

        // Progress tracking
        void SetGlobalProgressCallback(BundleProgressCallback callback);

    private:
        void RegisterAssetLoaders();
        bool ValidateAndResolveDependencies(const String& bundleName,
            std::vector<String>& loadOrder);
        void OnAssetLoadComplete(const LoadRequest& request);
        void UpdateBundleProgress(const String& bundleName);

        bool LoadBundleInternal(const String& bundleName, BundleProgressCallback callback);

        CachedResourceManager* m_resourceManager;
        Scope<BundleLoader> m_bundleLoader;
        Reference<AssetLoadQueue> m_loadQueue;
        Scope<AssetWorkerPool> m_workerPool;

        // Bundle tracking
        std::unordered_map<String, AssetBundleConfig> m_availableBundles;
        std::unordered_map<String, BundleLoadState> m_bundleStates;
        std::unordered_map<String, BundleProgressCallback> m_bundleCallbacks;

        // Asset ID to bundle mapping
        std::unordered_map<String, String> m_assetToBundleMap;

        void* m_context;

        BundleProgressCallback m_globalProgressCallback;
        mutable std::mutex m_bundlesMutex;
    };

    template <typename T>
    Reference<T> BundleManager::GetAsset(const String& assetId, void* context) {
        return m_resourceManager->GetResource(assetId, context);
    }
}