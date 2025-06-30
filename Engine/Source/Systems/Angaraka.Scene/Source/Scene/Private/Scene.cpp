module;

#include "Angaraka/Base.hpp"
#include "Angaraka/MathCore.hpp"
#include <algorithm>
#include <chrono>

module Angaraka.Scene;

import Angaraka.Scene.Component;
import Angaraka.Core.ResourceCache;
import Angaraka.Graphics.DirectX12;
import Angaraka.Scene.Components.MeshRenderer;

namespace Angaraka::SceneSystem {

    // ================== Scene Implementation ==================

    Scene::Scene(Angaraka::Core::CachedResourceManager* resourceManager, Angaraka::DirectX12GraphicsSystem* graphicsSystem)
        : m_resourceManager(resourceManager)
        , m_graphicsSystem(graphicsSystem) {
        AGK_ASSERT(resourceManager, "Scene: ResourceManager cannot be null!");
        AGK_ASSERT(graphicsSystem, "Scene: GraphicsSystem cannot be null!");

        AGK_INFO("Scene: Created scene '{}'", m_name);
    }

    Scene::~Scene() {
        AGK_INFO("Scene: Destroying scene '{}' with {} entities",
            m_name, m_entities.size());
        Clear();
    }

    // ================== Entity Management ==================

    Entity* Scene::CreateEntity(const String& name) {
        EntityID id = m_nextEntityId++;

        // Create entity
        auto entity = CreateScope<Entity>(id, this);
        Entity* entityPtr = entity.get();

        // Set name
        String entityName = name.empty() ? "Entity_" + std::to_string(id) : name;
        entityPtr->SetName(entityName);

        // Register transform mapping
        RegisterTransformMapping(&entityPtr->GetTransform(), entityPtr);

        // Store entity
        m_entities[id] = std::move(entity);

        // Update name mapping
        m_nameToEntities[entityName].push_back(id);

        // Update statistics
        if (m_collectStatistics) {
            m_statistics.totalEntities++;
            if (entityPtr->IsActive()) {
                m_statistics.activeEntities++;
            }
        }

        AGK_TRACE("Scene: Created entity '{}' with ID {}", entityName, id);

        // If scene has started, start the entity
        if (m_hasStarted && entityPtr->IsActive()) {
            entityPtr->Start();
        }

        return entityPtr;
    }

    Entity* Scene::CreateEntity(const Math::Vector3& position,
        const Math::Quaternion& rotation,
        const Math::Vector3& scale,
        const String& name) {
        Entity* entity = CreateEntity(name);
        if (entity) {
            entity->GetTransform().SetLocalPosition(position);
            entity->GetTransform().SetLocalRotation(rotation);
            entity->GetTransform().SetLocalScale(scale);
        }
        return entity;
    }

    void Scene::DestroyEntity(Entity* entity) {
        if (!entity) {
            return;
        }

        DestroyEntity(entity->GetID());
    }

    void Scene::DestroyEntity(EntityID id) {
        auto it = m_entities.find(id);
        if (it == m_entities.end()) {
            AGK_WARN("Scene: Cannot destroy entity with ID {} - not found", id);
            return;
        }

        Entity* entity = it->second.get();

        // Destroy all children first
        std::vector<SceneTransform*> childTransforms;
        entity->GetTransform().GetAllDescendants(childTransforms);

        for (SceneTransform* childTransform : childTransforms) {
            if (Entity* childEntity = GetEntityFromTransform(childTransform)) {
                DestroyEntityInternal(childEntity);
            }
        }

        // Destroy this entity
        DestroyEntityInternal(entity);
    }

    void Scene::DestroyEntityInternal(Entity* entity) {
        if (!entity) {
            return;
        }

        EntityID id = entity->GetID();
        String name = entity->GetName();
        EntityTag tag = entity->GetTag();

        AGK_TRACE("Scene: Destroying entity '{}' with ID {}", name, id);

        // Call OnDestroy
        entity->OnDestroy();

        // Remove from transform mapping
        UnregisterTransformMapping(&entity->GetTransform());

        // Remove from name mapping
        auto nameIt = m_nameToEntities.find(name);
        if (nameIt != m_nameToEntities.end()) {
            auto& ids = nameIt->second;
            ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
            if (ids.empty()) {
                m_nameToEntities.erase(nameIt);
            }
        }

        // Remove from tag mapping
        if (tag != 0) {
            auto tagIt = m_tagToEntities.find(tag);
            if (tagIt != m_tagToEntities.end()) {
                auto& ids = tagIt->second;
                ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
                if (ids.empty()) {
                    m_tagToEntities.erase(tagIt);
                }
            }
        }

        // Update statistics
        if (m_collectStatistics) {
            m_statistics.totalEntities--;
            if (entity->IsActive()) {
                m_statistics.activeEntities--;
            }
            m_statistics.componentCount -= static_cast<U32>(entity->GetComponents().size());
        }

        // Remove entity
        m_entities.erase(id);
    }

    Entity* Scene::FindEntity(EntityID id) const {
        auto it = m_entities.find(id);
        if (it != m_entities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    Entity* Scene::FindEntity(const String& name) const {
        auto it = m_nameToEntities.find(name);
        if (it != m_nameToEntities.end() && !it->second.empty()) {
            return FindEntity(it->second.front());
        }
        return nullptr;
    }

    void Scene::FindEntities(const String& name, std::vector<Entity*>& outEntities) const {
        auto it = m_nameToEntities.find(name);
        if (it != m_nameToEntities.end()) {
            for (EntityID id : it->second) {
                if (Entity* entity = FindEntity(id)) {
                    outEntities.push_back(entity);
                }
            }
        }
    }

    void Scene::FindEntitiesWithTag(EntityTag tag, std::vector<Entity*>& outEntities) const {
        auto it = m_tagToEntities.find(tag);
        if (it != m_tagToEntities.end()) {
            for (EntityID id : it->second) {
                if (Entity* entity = FindEntity(id)) {
                    outEntities.push_back(entity);
                }
            }
        }
    }

    void Scene::GetRootEntities(std::vector<Entity*>& outEntities) const {
        for (const auto& [id, entity] : m_entities) {
            if (entity->GetTransform().GetParent() == nullptr) {
                outEntities.push_back(entity.get());
            }
        }
    }

    // ================== Transform Mapping ==================

    Entity* Scene::GetEntityFromTransform(SceneTransform* transform) const {
        auto it = m_transformToEntity.find(transform);
        if (it != m_transformToEntity.end()) {
            return it->second;
        }
        return nullptr;
    }

    void Scene::RegisterTransformMapping(SceneTransform* transform, Entity* entity) {
        if (transform && entity) {
            m_transformToEntity[transform] = entity;
        }
    }

    void Scene::UnregisterTransformMapping(SceneTransform* transform) {
        m_transformToEntity.erase(transform);
    }

    // ================== Spatial Queries ==================

    void Scene::GetVisibleEntities(const Math::Frustum& frustum,
        std::vector<Entity*>& outEntities) const {
        // TODO: Use spatial acceleration structure (octree) when implemented
        // For now, brute force check all entities

        for (const auto& [id, entity] : m_entities) {
            if (!entity->IsActive()) {
                continue;
            }

            // TODO: Get bounding box from MeshRenderer component
            // For now, just add all active entities
            outEntities.push_back(entity.get());
        }

        if (m_collectStatistics) {
            m_statistics.visibleEntities = static_cast<U32>(outEntities.size());
            m_statistics.culledEntities = m_statistics.activeEntities - m_statistics.visibleEntities;
        }
    }

    void Scene::GetEntitiesInRadius(const Math::Vector3& center, F32 radius,
        std::vector<Entity*>& outEntities) const {
        F32 radiusSq = radius * radius;

        for (const auto& [id, entity] : m_entities) {
            if (!entity->IsActive()) {
                continue;
            }

            Math::Vector3 pos = entity->GetTransform().GetWorldPosition();
            F32 distSq = (pos - center).LengthSquared();

            if (distSq <= radiusSq) {
                outEntities.push_back(entity.get());
            }
        }
    }

    void Scene::GetEntitiesInBounds(const Math::BoundingBox& bounds,
        std::vector<Entity*>& outEntities) const {
        for (const auto& [id, entity] : m_entities) {
            if (!entity->IsActive()) {
                continue;
            }

            // TODO: Get bounding box from MeshRenderer component
            Math::Vector3 pos = entity->GetTransform().GetWorldPosition();
            if (bounds.Contains(pos)) {
                outEntities.push_back(entity.get());
            }
        }
    }

    bool Scene::Raycast(const Math::Ray& ray, F32 maxDistance, U32 layerMask,
        RaycastHit& outHit) const {
        std::vector<RaycastHit> hits;
        RaycastAll(ray, maxDistance, layerMask, hits);

        if (!hits.empty()) {
            // Return closest hit
            outHit = *std::min_element(hits.begin(), hits.end(),
                [](const RaycastHit& a, const RaycastHit& b) {
                    return a.distance < b.distance;
                });
            return true;
        }

        return false;
    }

    void Scene::RaycastAll(const Math::Ray& ray, F32 maxDistance, U32 layerMask,
        std::vector<RaycastHit>& outHits) const {
        // TODO: Implement proper raycasting against colliders
        // For now, this is a placeholder
        AGK_WARN("Scene::RaycastAll not fully implemented - requires collider components");
    }

    // ================== Scene Lifecycle ==================

    void Scene::Start() {
        if (m_hasStarted) {
            return;
        }

        AGK_INFO("Scene: Starting scene '{}'", m_name);
        m_hasStarted = true;

        // Start all entities
        for (auto& [id, entity] : m_entities) {
            if (entity->IsActive()) {
                entity->Start();
            }
        }
    }

    void Scene::Update(F32 deltaTime) {
        auto startTime = std::chrono::high_resolution_clock::now();

        // Update all active entities
        for (auto& [id, entity] : m_entities) {
            if (entity->IsActive()) {
                entity->Update(deltaTime);
            }
        }

        if (m_collectStatistics) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                endTime - startTime);
            m_statistics.updateTimeMs = duration.count() / 1000.0f;
        }
    }

    void Scene::LateUpdate(F32 deltaTime) {
        // Late update all active entities
        for (auto& [id, entity] : m_entities) {
            if (entity->IsActive()) {
                entity->LateUpdate(deltaTime);
            }
        }
    }

    void Scene::FixedUpdate(F32 fixedDeltaTime) {
        // Fixed update all active entities
        for (auto& [id, entity] : m_entities) {
            if (entity->IsActive()) {
                entity->FixedUpdate(fixedDeltaTime);
            }
        }
    }

    void Scene::PrepareRender(const Math::Vector3& cameraPosition,
        const Math::Frustum& frustum) {
        auto startTime = std::chrono::high_resolution_clock::now();

        // Clear render queues
        for (auto& queue : m_renderQueues) {
            queue.clear();
        }

        // Collect visible renderables
        CollectRenderables(frustum);

        // Sort render queues
        SortRenderQueues(cameraPosition);

        if (m_collectStatistics) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                endTime - startTime);
            m_statistics.cullingTimeMs = duration.count() / 1000.0f;
        }
    }

    const std::vector<Scene::RenderEntry>& Scene::GetRenderQueue(RenderQueueType queueType) const {
        size_t index = static_cast<size_t>(queueType);
        AGK_ASSERT(index < m_renderQueues.size(), "Invalid render queue type");
        return m_renderQueues[index];
    }

    // ================== Scene Management ==================

    void Scene::Clear() {
        AGK_INFO("Scene: Clearing all {} entities", m_entities.size());

        // Destroy all entities
        while (!m_entities.empty()) {
            DestroyEntity(m_entities.begin()->first);
        }

        // Clear mappings
        m_nameToEntities.clear();
        m_tagToEntities.clear();
        m_transformToEntity.clear();

        // Clear render queues
        for (auto& queue : m_renderQueues) {
            queue.clear();
        }

        // Reset ID counter
        m_nextEntityId = 1;
        m_hasStarted = false;

        // Reset statistics
        m_statistics = {};
    }

    bool Scene::LoadFromFile(const String& filePath) {
        AGK_INFO("Scene: Loading from '{}'", filePath);

        // TODO: Implement scene deserialization
        AGK_ERROR("Scene::LoadFromFile not yet implemented");
        return false;
    }

    bool Scene::SaveToFile(const String& filePath) const {
        AGK_INFO("Scene: Saving to '{}'", filePath);

        // TODO: Implement scene serialization
        AGK_ERROR("Scene::SaveToFile not yet implemented");
        return false;
    }

    // ================== Private Helper Methods ==================

    void Scene::CollectRenderables(const Math::Frustum& frustum) {
        for (auto& [id, entity] : m_entities) {
            if (!entity->IsActive()) {
                continue;
            }

            // Check if entity has a MeshRenderer component
            if (MeshRenderer* meshRenderer = entity->GetComponent<MeshRenderer>()) {
                // Check if mesh renderer is enabled
                if (!meshRenderer->IsEnabled()) {
                    continue;
                }

                // Frustum culling
                if (!meshRenderer->IsVisibleInFrustum(frustum)) {
                    if (m_collectStatistics) {
                        m_statistics.culledEntities++;
                    }
                    continue;
                }

                // Add to appropriate render queue
                RenderEntry entry;
                entry.entity = entity.get();
                entry.renderOrder = meshRenderer->GetRenderLayer();

                // For now, assume all meshes are opaque
                // TODO: Check material properties to determine queue
                m_renderQueues[static_cast<size_t>(RenderQueueType::Opaque)].push_back(entry);
            }
        }
    }

    void Scene::ExecuteRendering(DirectX12GraphicsSystem* renderer) {
        if (!renderer) {
            AGK_ERROR("Scene::ExecuteRendering - Renderer is null!");
            return;
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        // Render each queue in order
        for (size_t i = 0; i < static_cast<size_t>(RenderQueueType::Count); ++i) {
            const auto& queue = m_renderQueues[i];

            for (const RenderEntry& entry : queue) {
                Entity* entity = entry.entity;

                // Get MeshRenderer component
                if (MeshRenderer* meshRenderer = entity->GetComponent<MeshRenderer>()) {
                    // Get mesh resource
                    if (Core::Resource* meshResource = meshRenderer->GetMeshResource()) {
                        // Get world transform matrix
                        Math::Matrix4x4 worldMatrix = entity->GetTransform().GetWorldMatrix();

                        // Call the renderer
                        renderer->RenderMesh(meshResource, worldMatrix);
                    }
                }
            }
        }

        if (m_collectStatistics) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                endTime - startTime);
            m_statistics.renderTimeMs = duration.count() / 1000.0f;
        }
    }

    void Scene::SortRenderQueues(const Math::Vector3& cameraPosition) {
        // Sort opaque queue (front to back for early Z rejection)
        auto& opaqueQueue = m_renderQueues[static_cast<size_t>(RenderQueueType::Opaque)];
        std::sort(opaqueQueue.begin(), opaqueQueue.end(),
            [&cameraPosition](const RenderEntry& a, const RenderEntry& b) {
                // First sort by render order (layer)
                if (a.renderOrder != b.renderOrder) {
                    return a.renderOrder < b.renderOrder;
                }

                // Then sort by distance (front to back)
                F32 distA = (a.entity->GetTransform().GetWorldPosition() - cameraPosition).LengthSquared();
                F32 distB = (b.entity->GetTransform().GetWorldPosition() - cameraPosition).LengthSquared();
                return distA < distB;
            });

        // Sort transparent queue (back to front for proper blending)
        auto& transparentQueue = m_renderQueues[static_cast<size_t>(RenderQueueType::Transparent)];
        std::sort(transparentQueue.begin(), transparentQueue.end(),
            [&cameraPosition](const RenderEntry& a, const RenderEntry& b) {
                // First sort by render order (layer)
                if (a.renderOrder != b.renderOrder) {
                    return a.renderOrder < b.renderOrder;
                }

                // Then sort by distance (back to front)
                F32 distA = (a.entity->GetTransform().GetWorldPosition() - cameraPosition).LengthSquared();
                F32 distB = (b.entity->GetTransform().GetWorldPosition() - cameraPosition).LengthSquared();
                return distA > distB;
            });

        // Update distances in entries
        for (auto& queue : m_renderQueues) {
            for (auto& entry : queue) {
                entry.distanceToCamera = (entry.entity->GetTransform().GetWorldPosition()
                    - cameraPosition).Length();
            }
        }
    }

    void Scene::UpdateStatistics() {
        if (!m_collectStatistics) {
            return;
        }

        m_statistics.totalEntities = static_cast<U32>(m_entities.size());
        m_statistics.activeEntities = 0;
        m_statistics.componentCount = 0;

        for (const auto& [id, entity] : m_entities) {
            if (entity->IsActive()) {
                m_statistics.activeEntities++;
            }
            m_statistics.componentCount += static_cast<U32>(entity->GetComponents().size());
        }
    }

} // namespace Angaraka::Scene