module;

#include "Angaraka/Base.hpp"

module Angaraka.Core.Resources;

import Angaraka.Core.Events;

namespace Angaraka::Core {

    ResourceManager::ResourceManager(Angaraka::Events::EventManager& eventBus)
        : m_eventBus(eventBus)
    {
        AGK_INFO("ResourceManager: Initialized.");
        // Potentially subscribe to events here (e.g., asset hot-reload notifications)
    }

    ResourceManager::~ResourceManager() {
        AGK_INFO("ResourceManager: Shutting down. Unloading all resources...");
        UnloadAllResources(); // Ensure all resources are properly unloaded
    }

    void ResourceManager::UnloadResource(const std::string& id) {
        std::lock_guard<std::mutex> lock(m_resourcesMutex);
        auto it = m_loadedResources.find(id);
        if (it != m_loadedResources.end()) {
            // Call Unload() on the resource before removing it from the map
            it->second->Unload();
            m_loadedResources.erase(it);
            AGK_INFO("ResourceManager: Unloaded resource '{0}'.", id);
        }
        else {
            AGK_WARN("ResourceManager: Attempted to unload non-existent resource '{0}'.", id);
        }
    }

    void ResourceManager::UnloadAllResources() {
        std::lock_guard<std::mutex> lock(m_resourcesMutex);
        for (auto const& [id, resource] : m_loadedResources) {
            resource->Unload(); // Call unload on each resource
            AGK_INFO("ResourceManager: Unloaded all resource '{0}'.", id);
        }
        m_loadedResources.clear();
    }

    // LoadResourceInternal would typically be more complex, e.g., a factory pattern
    // that maps type IDs to actual loading functions or constructors.
    // For now, GetResource template handles the creation, so this might be removed
    // or refactored for more complex async loading.
    std::shared_ptr<Resource> ResourceManager::LoadResourceInternal(size_t typeId, const std::string& filePath) {
        // This is a placeholder. In a real engine, this would involve:
        // 1. Determining the resource type based on typeId or file extension.
        // 2. Calling the appropriate loader (e.g., TextureLoader, MeshLoader).
        // 3. Creating and returning a shared_ptr<Resource>.
        AGK_ERROR("ResourceManager::LoadResourceInternal: Generic internal loading not implemented for type ID {0}, path '{1}'.", typeId, filePath);
        return nullptr;
    }
} // namespace Angaraka::Core