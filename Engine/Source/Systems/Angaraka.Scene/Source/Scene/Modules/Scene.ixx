module;

#include "Angaraka/Base.hpp"
#include "Angaraka/MathCore.hpp"
#include <unordered_map>
#include <vector>
#include <functional>

export module Angaraka.Scene;

import Angaraka.Scene.Transform;
import Angaraka.Scene.Component;
import Angaraka.Scene.Entity;
import Angaraka.Graphics.DirectX12;
import Angaraka.Core.ResourceCache;

namespace Angaraka::SceneSystem {

    /**
     * @brief Raycast hit information
     */
    export struct RaycastHit {
        Entity* entity = nullptr;
        Component* component = nullptr;
        Math::Vector3 point;
        Math::Vector3 normal;
        F32 distance = 0.0f;
    };

    /**
     * @brief Scene contains and manages all entities
     *
     * The Scene class is responsible for:
     * - Creating and destroying entities
     * - Managing entity lifecycle (Start, Update, etc.)
     * - Spatial queries and culling
     * - Rendering organization
     * - Scene serialization
     */
    export class Scene {
    public:
        /**
         * @brief Scene statistics for performance monitoring
         */
        struct Statistics {
            U32 totalEntities = 0;
            U32 activeEntities = 0;
            U32 visibleEntities = 0;
            U32 culledEntities = 0;
            U32 componentCount = 0;
            F32 updateTimeMs = 0.0f;
            F32 cullingTimeMs = 0.0f;
            F32 renderTimeMs = 0.0f;
        };

        /**
         * @brief Render queue entry
         */
        struct RenderEntry {
            Entity* entity = nullptr;
            F32 distanceToCamera = 0.0f;
            U32 renderOrder = 0;
        };

        /**
         * @brief Render queue types
         */
        enum class RenderQueueType {
            Background = 0,    // Skybox, distant objects
            Opaque = 1,       // Normal opaque geometry
            Transparent = 2,  // Transparent objects (back to front)
            Overlay = 3,      // UI, debug overlays
            Count
        };

        Scene(Core::CachedResourceManager* resourceManager, DirectX12GraphicsSystem* graphicsSystem);
        ~Scene();

        // Non-copyable, non-moveable
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) = delete;
        Scene& operator=(Scene&&) = delete;

        // ================== Entity Management ==================

        /**
         * @brief Create a new entity
         * @param name Optional name for the entity
         * @return Pointer to created entity (owned by scene)
         */
        Entity* CreateEntity(const String& name = "");

        /**
         * @brief Create entity with transform
         * @param position Initial position
         * @param rotation Initial rotation
         * @param scale Initial scale
         * @param name Optional name
         */
        Entity* CreateEntity(const Math::Vector3& position,
            const Math::Quaternion& rotation = Math::Quaternion::Identity(),
            const Math::Vector3& scale = Math::Vector3(1, 1, 1),
            const String& name = "");

        /**
         * @brief Destroy an entity and all its children
         * @param entity Entity to destroy
         */
        void DestroyEntity(Entity* entity);

        /**
         * @brief Destroy entity by ID
         */
        void DestroyEntity(EntityID id);

        /**
         * @brief Find entity by ID
         * @return Entity pointer or nullptr
         */
        Entity* FindEntity(EntityID id) const;

        /**
         * @brief Find entity by name
         * @param name Name to search for
         * @return First entity with matching name or nullptr
         */
        Entity* FindEntity(const String& name) const;

        /**
         * @brief Find all entities with name
         * @param name Name to search for
         * @param outEntities Vector to fill with results
         */
        void FindEntities(const String& name, std::vector<Entity*>& outEntities) const;

        /**
         * @brief Find all entities with tag
         */
        void FindEntitiesWithTag(EntityTag tag, std::vector<Entity*>& outEntities) const;

        /**
         * @brief Get all entities
         */
        const std::unordered_map<EntityID, Scope<Entity>>& GetAllEntities() const {
            return m_entities;
        }

        /**
         * @brief Get root entities (no parent)
         */
        void GetRootEntities(std::vector<Entity*>& outEntities) const;

        // ================== Transform Mapping ==================

        /**
         * @brief Get entity that owns a transform
         * @param transform Transform to look up
         * @return Entity pointer or nullptr
         */
        Entity* GetEntityFromTransform(SceneTransform* transform) const;

        /**
         * @brief Register transform-entity mapping (internal use)
         */
        void RegisterTransformMapping(SceneTransform* transform, Entity* entity);

        /**
         * @brief Unregister transform-entity mapping (internal use)
         */
        void UnregisterTransformMapping(SceneTransform* transform);

        // ================== Spatial Queries ==================

        /**
         * @brief Get all entities visible in frustum
         * @param frustum Camera frustum
         * @param outEntities Vector to fill with visible entities
         */
        void GetVisibleEntities(const Math::Frustum& frustum,
            std::vector<Entity*>& outEntities) const;

        /**
         * @brief Get entities within radius
         * @param center Center point
         * @param radius Search radius
         * @param outEntities Vector to fill with results
         */
        void GetEntitiesInRadius(const Math::Vector3& center, F32 radius,
            std::vector<Entity*>& outEntities) const;

        /**
         * @brief Get entities within bounding box
         */
        void GetEntitiesInBounds(const Math::BoundingBox& bounds,
            std::vector<Entity*>& outEntities) const;

        /**
         * @brief Raycast into scene
         * @param ray Ray to cast
         * @param maxDistance Maximum ray distance
         * @param layerMask Layers to check (bit mask)
         * @param outHit Hit information
         * @return True if hit something
         */
        bool Raycast(const Math::Ray& ray, F32 maxDistance, U32 layerMask,
            RaycastHit& outHit) const;

        /**
         * @brief Raycast all hits
         */
        void RaycastAll(const Math::Ray& ray, F32 maxDistance, U32 layerMask,
            std::vector<RaycastHit>& outHits) const;

        // ================== Scene Lifecycle ==================

        /**
         * @brief Start the scene (calls Start on all entities)
         */
        void Start();

        /**
         * @brief Update all entities
         * @param deltaTime Time since last frame
         */
        void Update(F32 deltaTime);

        /**
         * @brief Late update all entities
         * @param deltaTime Time since last frame
         */
        void LateUpdate(F32 deltaTime);

        /**
         * @brief Fixed update for physics
         * @param fixedDeltaTime Fixed timestep
         */
        void FixedUpdate(F32 fixedDeltaTime);

        /**
         * @brief Prepare scene for rendering
         * @param cameraPosition Camera position for sorting
         * @param frustum Camera frustum for culling
         */
        void PrepareRender(const Math::Vector3& cameraPosition,
            const Math::Frustum& frustum);

        /**
         * @brief Execute rendering for all visible entities
         * @param renderer Graphics system to use for rendering
         */
        void ExecuteRendering(DirectX12GraphicsSystem* renderer);

        /**
         * @brief Get render queue
         * @param queueType Type of queue
         * @return Sorted render entries
         */
        const std::vector<RenderEntry>& GetRenderQueue(RenderQueueType queueType) const;

        // ================== Scene Management ==================

        /**
         * @brief Clear all entities
         */
        void Clear();

        /**
         * @brief Load scene from file
         * @param filePath Path to scene file
         * @return True if successful
         */
        bool LoadFromFile(const String& filePath);

        /**
         * @brief Save scene to file
         * @param filePath Path to save to
         * @return True if successful
         */
        bool SaveToFile(const String& filePath) const;

        /**
         * @brief Set scene name
         */
        void SetName(const String& name) { m_name = name; }
        const String& GetName() const { return m_name; }

        // ================== Statistics ==================

        /**
         * @brief Get scene statistics
         */
        const Statistics& GetStatistics() const { return m_statistics; }

        /**
         * @brief Enable/disable statistics collection
         */
        void SetStatisticsEnabled(bool enabled) { m_collectStatistics = enabled; }

        // ================== Systems Access ==================

        inline Core::CachedResourceManager* GetResourceManager() const {
            return m_resourceManager;
        }

        inline DirectX12GraphicsSystem* GetGraphicsSystem() const {
            return m_graphicsSystem;
        }

    private:
        // Core systems
        Core::CachedResourceManager* m_resourceManager;
        DirectX12GraphicsSystem* m_graphicsSystem;

        // Entity storage
        std::unordered_map<EntityID, Scope<Entity>> m_entities;
        std::unordered_map<String, std::vector<EntityID>> m_nameToEntities;
        std::unordered_map<EntityTag, std::vector<EntityID>> m_tagToEntities;
        EntityID m_nextEntityId = 1;

        // Transform mapping
        std::unordered_map<SceneTransform*, Entity*> m_transformToEntity;

        // Render queues
        std::array<std::vector<RenderEntry>,
            static_cast<size_t>(RenderQueueType::Count)> m_renderQueues;

        // Scene properties
        String m_name = "Untitled Scene";
        bool m_hasStarted = false;

        // Statistics
        mutable Statistics m_statistics;
        bool m_collectStatistics = true;

        // Spatial acceleration (future)
        // Scope<Octree> m_octree;

        // Helper methods
        void DestroyEntityInternal(Entity* entity);
        void CollectRenderables(const Math::Frustum& frustum);
        void SortRenderQueues(const Math::Vector3& cameraPosition);
        void UpdateStatistics();
    };

} // namespace Angaraka::Scene