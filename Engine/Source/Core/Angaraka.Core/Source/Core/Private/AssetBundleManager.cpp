#include "Angaraka/AssetBundleManager.hpp"
#include "Angaraka/Log.hpp"
#include <algorithm>
#include <unordered_set>

import Angaraka.Core.ResourceCache;

namespace Angaraka::Core {

    BundleManager::BundleManager(CachedResourceManager* resourceManager, void* context)
        : m_resourceManager(resourceManager)
        , m_context(context) {

        m_bundleLoader = CreateScope<BundleLoader>();
        m_loadQueue = CreateReference<AssetLoadQueue>();
        m_workerPool = CreateScope<AssetWorkerPool>(resourceManager);

        m_workerPool->SetLoadQueue(m_loadQueue);
        RegisterAssetLoaders();

        AGK_INFO("BundleManager: Initialized");
    }

    BundleManager::~BundleManager() {
        Shutdown();
    }

    bool BundleManager::Initialize(const std::filesystem::path& bundlesDirectory) {
        auto bundles = m_bundleLoader->LoadAllBundles(bundlesDirectory);

        std::lock_guard<std::mutex> lock(m_bundlesMutex);
        for (const auto& bundle : bundles) {
            String validationError;
            if (!m_bundleLoader->ValidateBundle(bundle, validationError)) {
                AGK_ERROR("Invalid bundle {}: {}", bundle.name, validationError);
                continue;
            }

            m_availableBundles[bundle.name] = bundle;
            m_bundleStates[bundle.name] = BundleLoadState::NotLoaded;

            // Map assets to bundles
            for (const auto& asset : bundle.assets) {
                m_assetToBundleMap[asset.id] = bundle.name;
            }
        }

        AGK_INFO("BundleManager: Loaded {} bundle definitions", m_availableBundles.size());
        return !m_availableBundles.empty();
    }

    void BundleManager::Shutdown() {
        StopAsyncLoading();

        std::lock_guard<std::mutex> lock(m_bundlesMutex);
        m_availableBundles.clear();
        m_bundleStates.clear();
        m_bundleCallbacks.clear();
        m_assetToBundleMap.clear();
    }

    bool BundleManager::LoadBundle(const String& bundleName, BundleProgressCallback callback) {
        std::lock_guard<std::mutex> lock(m_bundlesMutex);
        return LoadBundleInternal(bundleName, callback);
    }

    bool BundleManager::LoadBundleInternal(const String& bundleName, BundleProgressCallback callback) {
        // NOTE: Assumes m_bundlesMutex is already locked by caller

        auto it = m_availableBundles.find(bundleName);
        if (it == m_availableBundles.end()) {
            AGK_ERROR("Bundle not found: {}", bundleName);
            return false;
        }

        if (m_bundleStates[bundleName] == BundleLoadState::Loaded) {
            AGK_INFO("Bundle already loaded: {}", bundleName);
            return true;
        }

        // Resolve dependencies
        std::vector<String> loadOrder;
        if (!ValidateAndResolveDependencies(bundleName, loadOrder)) {
            return false;
        }

        // Set callback
        if (callback) {
            m_bundleCallbacks[bundleName] = callback;
        }

        // Load dependencies first (recursive calls without locking)
        for (const auto& depName : loadOrder) {
            if (depName != bundleName && m_bundleStates[depName] != BundleLoadState::Loaded) {
                if (!LoadBundleInternal(depName, nullptr)) {  // Use internal method
                    AGK_ERROR("Failed to load dependency: {}", depName);
                    return false;
                }
            }
        }

        // Queue bundle assets
        const auto& bundle = it->second;
        m_bundleStates[bundleName] = BundleLoadState::Loading;

        auto completeCallback = [this](const LoadRequest& request) {
            OnAssetLoadComplete(request);
            };

        m_loadQueue->EnqueueBundle(bundle, completeCallback);

        AGK_INFO("Bundle queued for loading: {}", bundleName);
        return true;
    }

    bool BundleManager::UnloadBundle(const String& bundleName) {
        std::lock_guard<std::mutex> lock(m_bundlesMutex);

        auto it = m_availableBundles.find(bundleName);
        if (it == m_availableBundles.end()) {
            return false;
        }

        const auto& bundle = it->second;
        if (bundle.unloadStrategy == UnloadStrategy::Manual) {
            // Unload all assets in bundle
            for (const auto& asset : bundle.assets) {
                m_resourceManager->UnloadResource(asset.id);
            }

            m_bundleStates[bundleName] = BundleLoadState::NotLoaded;
            AGK_INFO("Bundle unloaded: {}", bundleName);
            return true;
        }

        AGK_WARN("Cannot manually unload automatic bundle: {}", bundleName);
        return false;
    }

    void BundleManager::LoadAllAutoLoadBundles() {
        std::vector<String> autoLoadBundles;

        {
            std::lock_guard<std::mutex> lock(m_bundlesMutex);
            for (const auto& [name, bundle] : m_availableBundles) {
                if (bundle.autoLoad) {
                    autoLoadBundles.push_back(name);
                }
            }
        }

        for (const auto& bundleName : autoLoadBundles) {
            LoadBundle(bundleName);
        }

        AGK_INFO("Queued {} auto-load bundles", autoLoadBundles.size());
    }

    void BundleManager::StartAsyncLoading() {
        m_workerPool->Start();
        AGK_INFO("Async asset loading started");
    }

    void BundleManager::StopAsyncLoading() {
        m_workerPool->Stop();
        AGK_INFO("Async asset loading stopped");
    }

    void BundleManager::PauseAsyncLoading() {
        m_workerPool->PauseWorkers();
    }

    void BundleManager::ResumeAsyncLoading() {
        m_workerPool->ResumeWorkers();
    }

    void BundleManager::RegisterAssetLoaders() {
        auto loader = [this](const AssetDefinition& asset) -> Reference<Resource> {
            return m_resourceManager->GetResource<Resource>(asset.id, asset.path, m_context);
        };

        // Register texture loader
        m_workerPool->RegisterLoader(AssetType::Texture, loader);
        m_workerPool->RegisterLoader(AssetType::Mesh, loader);

        // TODO: Register mesh, material, sound loaders
        AGK_INFO("Asset loaders registered");
    }

    bool BundleManager::ValidateAndResolveDependencies(const String& bundleName,
        std::vector<String>& loadOrder) {
        std::unordered_set<String> visited;
        std::unordered_set<String> inProgress;

        std::function<bool(const String&)> resolveDeps = [&](const String& name) -> bool {
            if (inProgress.count(name)) {
                AGK_ERROR("Circular dependency detected: {}", name);
                return false;
            }

            if (visited.count(name)) {
                return true;
            }

            auto it = m_availableBundles.find(name);
            if (it == m_availableBundles.end()) {
                AGK_ERROR("Dependency not found: {}", name);
                return false;
            }

            inProgress.insert(name);

            for (const auto& dep : it->second.dependencies) {
                if (!resolveDeps(dep)) {
                    return false;
                }
            }

            inProgress.erase(name);
            visited.insert(name);
            loadOrder.push_back(name);
            return true;
            };

        return resolveDeps(bundleName);
    }

    void BundleManager::OnAssetLoadComplete(const LoadRequest& request) {
        UpdateBundleProgress(request.bundleName);
    }

    void BundleManager::UpdateBundleProgress(const String& bundleName) {
        std::lock_guard<std::mutex> lock(m_bundlesMutex);

        auto bundleIt = m_availableBundles.find(bundleName);
        if (bundleIt == m_availableBundles.end()) return;

        const auto& bundle = bundleIt->second;
        size_t totalAssets = bundle.assets.size();
        size_t loadedAssets = 0;

        for (const auto& asset : bundle.assets) {
            auto status = m_loadQueue->GetAssetStatus(asset.id);
            if (status == LoadStatus::Completed) {
                loadedAssets++;
            }
        }

        BundleLoadProgress progress;
        progress.bundleName = bundleName;
        progress.assetsLoaded = loadedAssets;
        progress.totalAssets = totalAssets;
        progress.progress = totalAssets > 0 ? static_cast<F32>(loadedAssets) / totalAssets : 1.0f;
        progress.state = (loadedAssets == totalAssets) ? BundleLoadState::Loaded : BundleLoadState::Loading;

        if (progress.state == BundleLoadState::Loaded) {
            m_bundleStates[bundleName] = BundleLoadState::Loaded;
        }

        // Call callbacks
        auto callbackIt = m_bundleCallbacks.find(bundleName);
        if (callbackIt != m_bundleCallbacks.end()) {
            callbackIt->second(progress);
        }

        if (m_globalProgressCallback) {
            m_globalProgressCallback(progress);
        }
    }

    bool BundleManager::IsBundleLoaded(const String& bundleName) const {
        std::lock_guard<std::mutex> lock(m_bundlesMutex);
        auto it = m_bundleStates.find(bundleName);
        return it != m_bundleStates.end() && it->second == BundleLoadState::Loaded;
    }

    BundleLoadState BundleManager::GetBundleState(const String& bundleName) const {
        std::lock_guard<std::mutex> lock(m_bundlesMutex);
        auto it = m_bundleStates.find(bundleName);
        return it != m_bundleStates.end() ? it->second : BundleLoadState::NotLoaded;
    }

    BundleLoadProgress BundleManager::GetBundleProgress(const String& bundleName) const {
        std::lock_guard<std::mutex> lock(m_bundlesMutex);

        BundleLoadProgress progress;
        progress.bundleName = bundleName;

        auto bundleIt = m_availableBundles.find(bundleName);
        if (bundleIt == m_availableBundles.end()) {
            return progress;
        }

        auto stateIt = m_bundleStates.find(bundleName);
        progress.state = stateIt != m_bundleStates.end() ? stateIt->second : BundleLoadState::NotLoaded;

        const auto& bundle = bundleIt->second;
        progress.totalAssets = bundle.assets.size();

        for (const auto& asset : bundle.assets) {
            auto status = m_loadQueue->GetAssetStatus(asset.id);
            if (status == LoadStatus::Completed) {
                progress.assetsLoaded++;
            }
        }

        progress.progress = progress.totalAssets > 0 ?
            static_cast<F32>(progress.assetsLoaded) / progress.totalAssets : 1.0f;

        return progress;
    }

    std::vector<String> BundleManager::GetLoadedBundles() const {
        std::lock_guard<std::mutex> lock(m_bundlesMutex);

        std::vector<String> loaded;
        for (const auto& [name, state] : m_bundleStates) {
            if (state == BundleLoadState::Loaded) {
                loaded.push_back(name);
            }
        }
        return loaded;
    }

    std::vector<String> BundleManager::GetAvailableBundles() const {
        std::lock_guard<std::mutex> lock(m_bundlesMutex);

        std::vector<String> available;
        available.reserve(m_availableBundles.size());
        for (const auto& [name, bundle] : m_availableBundles) {
            available.push_back(name);
        }
        return available;
    }

    bool BundleManager::IsAssetLoaded(const String& assetId) const {
        auto status = m_loadQueue->GetAssetStatus(assetId);
        return status == LoadStatus::Completed;
    }

    void BundleManager::UnloadUnusedAssets() {
        std::lock_guard<std::mutex> lock(m_bundlesMutex);

        size_t unloaded = 0;
        for (const auto& [bundleName, bundle] : m_availableBundles) {
            if (bundle.unloadStrategy == UnloadStrategy::Automatic) {
                for (const auto& asset : bundle.assets) {
                    // Check if asset is still referenced (use_count > 1 means referenced elsewhere)
                    auto resource = m_resourceManager->GetResource<Resource>(asset.id);
                    if (resource && resource.use_count() <= 1) {
                        m_resourceManager->UnloadResource(asset.id);
                        unloaded++;
                    }
                }
            }
        }

        AGK_INFO("UnloadUnusedAssets: Unloaded {} unused assets", unloaded);
    }

    size_t BundleManager::GetMemoryUsage() const {
        return m_resourceManager->GetCacheMemoryUsage();
    }

    void BundleManager::SetGlobalProgressCallback(BundleProgressCallback callback) {
        std::lock_guard<std::mutex> lock(m_bundlesMutex);
        m_globalProgressCallback = callback;
    }
}