#include "Angaraka/AssetWorkerPool.hpp"
#include "Angaraka/Log.hpp"
#include <chrono>

#undef max

import Angaraka.Core.ResourceCache;

namespace Angaraka::Core {

    AssetWorkerPool::AssetWorkerPool(CachedResourceManager* resourceManager, size_t numThreads)
        : m_resourceManager(resourceManager)
        , m_numThreads(numThreads) {

        if (numThreads == 0) {
            m_numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
        }

        AGK_INFO("AssetWorkerPool: Initialized with {} threads", m_numThreads);
    }

    AssetWorkerPool::~AssetWorkerPool() {
        Stop();
    }

    void AssetWorkerPool::Start() {
        if (m_running.load()) {
            AGK_WARN("AssetWorkerPool already running");
            return;
        }

        m_running.store(true);
        m_shouldStop.store(false);
        m_paused.store(false);

        m_workers.reserve(m_numThreads);
        for (size_t i = 0; i < m_numThreads; ++i) {
            m_workers.emplace_back(&AssetWorkerPool::WorkerThreadMain, this, i);
        }

        AGK_INFO("AssetWorkerPool: Started {} worker threads", m_numThreads);
    }

    void AssetWorkerPool::Stop() {
        if (!m_running.load()) {
            return;
        }

        AGK_INFO("AssetWorkerPool: Stopping worker threads...");

        m_shouldStop.store(true);
        m_workCondition.notify_all();

        for (auto& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        m_workers.clear();
        m_running.store(false);
        AGK_INFO("AssetWorkerPool: All worker threads stopped");
    }

    void AssetWorkerPool::RegisterLoader(AssetType type, AssetLoaderFunction loader) {
        std::lock_guard<std::mutex> lock(m_loadersMutex);
        m_loaders[type] = loader;
        AGK_INFO("Registered loader for asset type: {}", AssetDefinition::AssetTypeToString(type));
    }

    void AssetWorkerPool::SetLoadQueue(Reference<AssetLoadQueue> queue) {
        std::lock_guard<std::mutex> lock(m_workMutex);
        m_loadQueue = queue;
        m_workCondition.notify_all();
    }

    void AssetWorkerPool::PauseWorkers() {
        m_paused.store(true);
        AGK_INFO("AssetWorkerPool: Workers paused");
    }

    void AssetWorkerPool::ResumeWorkers() {
        m_paused.store(false);
        m_workCondition.notify_all();
        AGK_INFO("AssetWorkerPool: Workers resumed");
    }

    void AssetWorkerPool::WorkerThreadMain(size_t threadId) {
        AGK_TRACE("Worker thread {} started", threadId);

        while (!m_shouldStop.load()) {
            try {
                if (m_paused.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                if (!ProcessNextAsset()) {
                    // No work available, wait for notification
                    std::unique_lock<std::mutex> lock(m_workMutex);
                    m_workCondition.wait_for(lock, std::chrono::milliseconds(100),
                        [this] { return m_shouldStop.load() || !m_paused.load(); });
                }
            }
            catch (const std::exception& e) {
                AGK_ERROR("Worker thread {} exception: {}", threadId, e.what());
            }
        }

        AGK_TRACE("Worker thread {} stopped", threadId);
    }

    bool AssetWorkerPool::ProcessNextAsset() {
        if (!m_loadQueue) {
            return false;
        }

        auto request = m_loadQueue->DequeueNextAsset();
        if (!request.has_value()) {
            return false; // No work available
        }

        const auto& asset = request->asset;
        AGK_TRACE("Worker processing asset: {}", asset.id);

        try {
            auto resource = LoadAsset(asset);

            if (resource) {
                m_loadQueue->MarkAssetCompleted(asset.id, resource);
                AGK_TRACE("Successfully loaded asset: {}", asset.id);
            }
            else {
                m_loadQueue->MarkAssetCompleted(asset.id, nullptr, "Failed to load asset");
                AGK_WARN("Failed to load asset: {}", asset.id);
            }
        }
        catch (const std::exception& e) {
            m_loadQueue->MarkAssetCompleted(asset.id, nullptr, e.what());
            AGK_ERROR("Exception loading asset {}: {}", asset.id, e.what());
        }

        return true;
    }

    Reference<Resource> AssetWorkerPool::LoadAsset(const AssetDefinition& asset) {
        std::lock_guard<std::mutex> lock(m_loadersMutex);

        auto it = m_loaders.find(asset.type);
        if (it == m_loaders.end()) {
            AGK_ERROR("No loader registered for asset type: {}",
                AssetDefinition::AssetTypeToString(asset.type));
            return nullptr;
        }

        return it->second(asset);
    }

}