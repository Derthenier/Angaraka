module;

#include "Angaraka/Base.hpp"
#include "Angaraka/Math/BoundingBox.hpp"
#include "Angaraka/Math/Frustum.hpp"
#include "Angaraka/Math/Ray.hpp"
#include <vector>
#include <array>
#include <functional>
#include <unordered_set>

export module Angaraka.Scene.Octree;

import Angaraka.Scene.Entity;
import Angaraka.Scene.Components.MeshRenderer;

namespace Angaraka::SceneSystem {

    /**
     * @brief Octree node for spatial partitioning
     *
     * Each node can have up to 8 children, dividing space into octants.
     * Entities are stored in leaf nodes up to a maximum count, then subdivided.
     */
    export class OctreeNode {
    public:
        /**
         * @brief Octant indices
         *
         * Bit pattern: [z][y][x]
         * 0 = negative, 1 = positive
         */
        enum Octant {
            NegXNegYNegZ = 0,  // 000
            PosXNegYNegZ = 1,  // 001
            NegXPosYNegZ = 2,  // 010
            PosXPosYNegZ = 3,  // 011
            NegXNegYPosZ = 4,  // 100
            PosXNegYPosZ = 5,  // 101
            NegXPosYPosZ = 6,  // 110
            PosXPosYPosZ = 7   // 111
        };

        OctreeNode(const Math::BoundingBox& bounds, U32 depth = 0);
        ~OctreeNode() = default;

        // Node properties
        const Math::BoundingBox& GetBounds() const { return m_bounds; }
        U32 GetDepth() const { return m_depth; }
        bool IsLeaf() const { return m_children[0] == nullptr; }
        size_t GetEntityCount() const { return m_entities.size(); }

        // Entity management
        void InsertEntity(Entity* entity);
        void RemoveEntity(Entity* entity);
        void UpdateEntity(Entity* entity);
        bool ContainsEntity(Entity* entity) const;

        // Spatial queries
        void GetEntitiesInFrustum(const Math::Frustum& frustum,
            std::vector<Entity*>& results) const;
        void GetEntitiesInBounds(const Math::BoundingBox& bounds,
            std::vector<Entity*>& results) const;
        void GetEntitiesInSphere(const Math::Vector3& center, F32 radius,
            std::vector<Entity*>& results) const;
        void GetEntitiesAlongRay(const Math::Ray& ray, F32 maxDistance,
            std::vector<Entity*>& results) const;

        // Child access
        OctreeNode* GetChild(Octant octant) { return m_children[octant].get(); }
        const OctreeNode* GetChild(Octant octant) const { return m_children[octant].get(); }

        // Subdivision
        void Subdivide();
        void Collapse();
        bool ShouldSubdivide() const;
        bool ShouldCollapse() const;

        // Utilities
        Octant GetOctantForPoint(const Math::Vector3& point) const;
        Math::BoundingBox GetOctantBounds(Octant octant) const;
        void GetAllEntities(std::vector<Entity*>& results) const;

        // Debug
        void GetNodeBounds(std::vector<Math::BoundingBox>& bounds, U32 maxDepth = U32(-1)) const;
        U32 GetNodeCount() const;
        U32 GetMaxDepth() const;

    private:
        // Node data
        Math::BoundingBox m_bounds;
        U32 m_depth;
        Math::Vector3 m_center;
        Math::Vector3 m_halfSize;

        // Children (only allocated when subdivided)
        std::array<Scope<OctreeNode>, 8> m_children;

        // Entities in this node (only in leaf nodes)
        std::vector<Entity*> m_entities;

        // Configuration (could be static or passed in)
        static constexpr U32 MAX_ENTITIES_PER_NODE = 16;
        static constexpr U32 MIN_ENTITIES_TO_COLLAPSE = 4;
        static constexpr F32 MIN_NODE_SIZE = 1.0f;

        // Helper methods
        void InsertIntoChildren(Entity* entity);
        void CollectFromChildren();
        bool FitsInOctant(Entity* entity, Octant octant) const;
        Math::BoundingBox GetEntityBounds(Entity* entity) const;
    };

    /**
     * @brief Octree spatial acceleration structure
     *
     * Manages a dynamic octree that automatically subdivides and collapses
     * based on entity distribution. Optimized for frustum culling and
     * spatial queries.
     */
    export class Octree {
    public:
        /**
         * @brief Configuration for octree behavior
         */
        struct Config {
            Math::BoundingBox worldBounds;      // Total world bounds
            U32 maxDepth = 8;                   // Maximum subdivision depth
            U32 maxEntitiesPerNode = 16;        // Entities before subdivision
            U32 minEntitiesPerNode = 4;         // Entities before collapse
            F32 minNodeSize = 1.0f;             // Minimum node size
            bool dynamicExpansion = true;       // Allow root expansion
            F32 expansionFactor = 2.0f;         // How much to expand
        };

        explicit Octree(const Config& config = Config());
        ~Octree() = default;

        // Entity management
        void Insert(Entity* entity);
        void Remove(Entity* entity);
        void Update(Entity* entity);
        void Clear();

        // Bulk operations
        void InsertMultiple(const std::vector<Entity*>& entities);
        void RemoveMultiple(const std::vector<Entity*>& entities);
        void UpdateAll();

        // Spatial queries
        void Query(const Math::Frustum& frustum, std::vector<Entity*>& results) const;
        void Query(const Math::BoundingBox& bounds, std::vector<Entity*>& results) const;
        void Query(const Math::Vector3& center, F32 radius, std::vector<Entity*>& results) const;
        void QueryRay(const Math::Ray& ray, F32 maxDistance, std::vector<Entity*>& results) const;

        // Nearest neighbor queries
        Entity* FindNearest(const Math::Vector3& point, F32 maxDistance = FLT_MAX) const;
        void FindKNearest(const Math::Vector3& point, U32 k, std::vector<Entity*>& results) const;

        // Maintenance
        void Optimize();
        void Rebuild();
        void ExpandToInclude(const Math::BoundingBox& bounds);

        // Properties
        const Math::BoundingBox& GetBounds() const { return m_root ? m_root->GetBounds() : m_config.worldBounds; }
        U32 GetEntityCount() const { return static_cast<U32>(m_entityToNode.size()); }
        bool IsEmpty() const { return m_entityToNode.empty(); }

        // Statistics
        struct Statistics {
            U32 totalNodes = 0;
            U32 leafNodes = 0;
            U32 maxDepth = 0;
            U32 totalEntities = 0;
            F32 averageEntitiesPerLeaf = 0.0f;
            U32 emptyLeafNodes = 0;
        };

        Statistics GetStatistics() const;

        // Debug visualization
        void GetAllNodeBounds(std::vector<Math::BoundingBox>& bounds, U32 maxDepth = U32(-1)) const;

        // Visitor pattern for custom operations
        template<typename Func>
        void TraverseNodes(Func&& visitor) const {
            if (m_root) {
                TraverseNodesRecursive(m_root.get(), std::forward<Func>(visitor));
            }
        }

    private:
        Config m_config;
        Scope<OctreeNode> m_root;

        // Track which node each entity is in for fast updates
        std::unordered_map<Entity*, OctreeNode*> m_entityToNode;

        // Pending updates (batched for efficiency)
        std::unordered_set<Entity*> m_pendingUpdates;
        bool m_batchMode = false;

        // Helper methods
        void RemoveFromNode(Entity* entity, OctreeNode* node);
        void InsertIntoNode(Entity* entity, OctreeNode* node);
        OctreeNode* FindBestNode(Entity* entity, OctreeNode* start) const;
        void CollapseEmptyNodes(OctreeNode* node);

        template<typename Func>
        void TraverseNodesRecursive(const OctreeNode* node, Func&& visitor) const {
            visitor(node);
            if (!node->IsLeaf()) {
                for (U32 i = 0; i < 8; ++i) {
                    if (auto* child = node->GetChild(static_cast<OctreeNode::Octant>(i))) {
                        TraverseNodesRecursive(child, visitor);
                    }
                }
            }
        }

        // Batch update support
        void BeginBatchUpdate() { m_batchMode = true; }
        void EndBatchUpdate();
    };

} // namespace Angaraka::Scene