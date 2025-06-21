#pragma once

#include "Angaraka/AssetLoadQueue.hpp"
#include "Angaraka/AssetBundleConfig.hpp"
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>

namespace Angaraka::Core {

    class CachedResourceManager;

    using AssetLoaderFunction = std::function<std::shared_ptr<Resource>(const AssetDefinition&)>;

    class AssetWorkerPool {
    public:
        explicit AssetWorkerPool(CachedResourceManager* resourceManager, size_t numThreads = 2);
        ~AssetWorkerPool();

        // Start/stop the worker threads
        void Start();
        void Stop();
        bool IsRunning() const { return m_running.load(); }

        // Register asset type loaders
        void RegisterLoader(AssetType type, AssetLoaderFunction loader);

        // Set the load queue to process
        void SetLoadQueue(std::shared_ptr<AssetLoadQueue> queue);

        // Worker thread control
        void PauseWorkers();
        void ResumeWorkers();

    private:
        void WorkerThreadMain(size_t threadId);
        bool ProcessNextAsset();
        std::shared_ptr<Resource> LoadAsset(const AssetDefinition& asset);

        CachedResourceManager* m_resourceManager;
        std::shared_ptr<AssetLoadQueue> m_loadQueue;

        size_t m_numThreads;
        std::vector<std::thread> m_workers;
        std::atomic<bool> m_running{ false };
        std::atomic<bool> m_paused{ false };
        std::atomic<bool> m_shouldStop{ false };

        mutable std::mutex m_workMutex;
        std::condition_variable m_workCondition;

        // Asset type loaders
        std::unordered_map<AssetType, AssetLoaderFunction> m_loaders;
        std::mutex m_loadersMutex;
    };

}
