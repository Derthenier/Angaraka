module;

#include "Angaraka/Base.hpp"
#include "Angaraka/Math/BoundingBox.hpp"
#include "Angaraka/Math/Frustum.hpp"
#include "Angaraka/Math/Ray.hpp"
#include <algorithm>
#include <queue>
#include <limits>

module Angaraka.Scene.Octree;

import Angaraka.Scene.Entity;
import Angaraka.Scene.Components.MeshRenderer;

namespace Angaraka::SceneSystem {

    // ================== OctreeNode Implementation ==================

    OctreeNode::OctreeNode(const Math::BoundingBox& bounds, U32 depth)
        : m_bounds(bounds)
        , m_depth(depth) {
        m_center = bounds.GetCenter();
        m_halfSize = bounds.GetHalfExtents();
    }

    void OctreeNode::InsertEntity(Entity* entity) {
        if (!entity) return;

        if (IsLeaf()) {
            // Add to this node
            m_entities.push_back(entity);

            // Check if we should subdivide
            if (ShouldSubdivide()) {
                Subdivide();
            }
        }
        else {
            // Insert into appropriate child
            InsertIntoChildren(entity);
        }
    }

    void OctreeNode::RemoveEntity(Entity* entity) {
        if (!entity) return;

        if (IsLeaf()) {
            auto it = std::find(m_entities.begin(), m_entities.end(), entity);
            if (it != m_entities.end()) {
                m_entities.erase(it);
            }
        }
        else {
            // Remove from all children (entity might span multiple)
            for (auto& child : m_children) {
                if (child) {
                    child->RemoveEntity(entity);
                }
            }

            // Check if we should collapse
            if (ShouldCollapse()) {
                Collapse();
            }
        }
    }

    void OctreeNode::UpdateEntity(Entity* entity) {
        // For now, just remove and re-insert
        // Could be optimized to check if entity moved octants
        RemoveEntity(entity);
        InsertEntity(entity);
    }

    bool OctreeNode::ContainsEntity(Entity* entity) const {
        if (IsLeaf()) {
            return std::find(m_entities.begin(), m_entities.end(), entity) != m_entities.end();
        }

        for (const auto& child : m_children) {
            if (child && child->ContainsEntity(entity)) {
                return true;
            }
        }
        return false;
    }

    void OctreeNode::GetEntitiesInFrustum(const Math::Frustum& frustum,
        std::vector<Entity*>& results) const {
        // Check if node intersects frustum
        auto intersection = frustum.IntersectsBoundingBox(m_bounds);
        if (intersection == Math::Frustum::IntersectionResult::Outside) {
            return;
        }

        if (IsLeaf()) {
            // Add all entities that are visible
            for (Entity* entity : m_entities) {
                if (!entity->IsActive()) continue;

                if (auto* renderer = entity->GetComponent<MeshRenderer>()) {
                    if (intersection == Math::Frustum::IntersectionResult::Inside ||
                        renderer->IsVisibleInFrustum(frustum)) {
                        results.push_back(entity);
                    }
                }
            }
        }
        else {
            // Recurse to children
            for (const auto& child : m_children) {
                if (child) {
                    child->GetEntitiesInFrustum(frustum, results);
                }
            }
        }
    }

    void OctreeNode::GetEntitiesInBounds(const Math::BoundingBox& bounds,
        std::vector<Entity*>& results) const {
        if (!m_bounds.Intersects(bounds)) {
            return;
        }

        if (IsLeaf()) {
            for (Entity* entity : m_entities) {
                if (!entity->IsActive()) continue;

                Math::BoundingBox entityBounds = GetEntityBounds(entity);
                if (bounds.Intersects(entityBounds)) {
                    results.push_back(entity);
                }
            }
        }
        else {
            for (const auto& child : m_children) {
                if (child) {
                    child->GetEntitiesInBounds(bounds, results);
                }
            }
        }
    }

    void OctreeNode::GetEntitiesInSphere(const Math::Vector3& center, F32 radius,
        std::vector<Entity*>& results) const {
        if (!m_bounds.IntersectsSphere(center, radius)) {
            return;
        }

        if (IsLeaf()) {
            F32 radiusSq = radius * radius;
            for (Entity* entity : m_entities) {
                if (!entity->IsActive()) continue;

                Math::Vector3 entityPos = entity->GetTransform().GetWorldPosition();
                F32 distSq = (entityPos - center).LengthSquared();
                if (distSq <= radiusSq) {
                    results.push_back(entity);
                }
            }
        }
        else {
            for (const auto& child : m_children) {
                if (child) {
                    child->GetEntitiesInSphere(center, radius, results);
                }
            }
        }
    }

    void OctreeNode::GetEntitiesAlongRay(const Math::Ray& ray, F32 maxDistance,
        std::vector<Entity*>& results) const {
        F32 tMin, tMax;
        if (!m_bounds.IntersectsRay(ray.origin, ray.direction, tMin, tMax)) {
            return;
        }

        if (tMin > maxDistance) {
            return;
        }

        if (IsLeaf()) {
            for (Entity* entity : m_entities) {
                if (!entity->IsActive()) continue;

                // Simple distance check for now
                // Could add more precise ray-entity intersection
                Math::Vector3 entityPos = entity->GetTransform().GetWorldPosition();
                F32 dist = ray.DistanceToPoint(entityPos);
                if (dist < 1.0f) { // Threshold
                    results.push_back(entity);
                }
            }
        }
        else {
            for (const auto& child : m_children) {
                if (child) {
                    child->GetEntitiesAlongRay(ray, maxDistance, results);
                }
            }
        }
    }

    void OctreeNode::Subdivide() {
        if (!IsLeaf()) return;

        // Create children
        for (U32 i = 0; i < 8; ++i) {
            Octant octant = static_cast<Octant>(i);
            Math::BoundingBox childBounds = GetOctantBounds(octant);
            m_children[i] = CreateScope<OctreeNode>(childBounds, m_depth + 1);
        }

        // Move entities to children
        std::vector<Entity*> entities = std::move(m_entities);
        m_entities.clear();

        for (Entity* entity : entities) {
            InsertIntoChildren(entity);
        }
    }

    void OctreeNode::Collapse() {
        if (IsLeaf()) return;

        // Collect all entities from children
        CollectFromChildren();

        // Delete children
        for (auto& child : m_children) {
            child.reset();
        }
    }

    bool OctreeNode::ShouldSubdivide() const {
        if (!IsLeaf()) return false;
        if (m_entities.size() <= MAX_ENTITIES_PER_NODE) return false;
        if (m_halfSize.x < MIN_NODE_SIZE || m_halfSize.y < MIN_NODE_SIZE || m_halfSize.z < MIN_NODE_SIZE) return false;
        return true;
    }

    bool OctreeNode::ShouldCollapse() const {
        if (IsLeaf()) return false;

        U32 totalEntities = 0;
        GetAllEntities(const_cast<std::vector<Entity*>&>(m_entities)); // Temporary hack
        totalEntities = static_cast<U32>(m_entities.size());
        const_cast<std::vector<Entity*>&>(m_entities).clear();

        return totalEntities <= MIN_ENTITIES_TO_COLLAPSE;
    }

    OctreeNode::Octant OctreeNode::GetOctantForPoint(const Math::Vector3& point) const {
        U32 index = 0;
        if (point.x > m_center.x) index |= 1;
        if (point.y > m_center.y) index |= 2;
        if (point.z > m_center.z) index |= 4;
        return static_cast<Octant>(index);
    }

    Math::BoundingBox OctreeNode::GetOctantBounds(Octant octant) const {
        Math::Vector3 newHalfSize = m_halfSize * 0.5f;
        Math::Vector3 offset;

        offset.x = (octant & 1) ? newHalfSize.x : -newHalfSize.x;
        offset.y = (octant & 2) ? newHalfSize.y : -newHalfSize.y;
        offset.z = (octant & 4) ? newHalfSize.z : -newHalfSize.z;

        Math::Vector3 newCenter = m_center + offset;

        return Math::BoundingBox(
            newCenter - newHalfSize,
            newCenter + newHalfSize
        );
    }

    void OctreeNode::GetAllEntities(std::vector<Entity*>& results) const {
        if (IsLeaf()) {
            results.insert(results.end(), m_entities.begin(), m_entities.end());
        }
        else {
            for (const auto& child : m_children) {
                if (child) {
                    child->GetAllEntities(results);
                }
            }
        }
    }

    void OctreeNode::GetNodeBounds(std::vector<Math::BoundingBox>& bounds, U32 maxDepth) const {
        if (m_depth <= maxDepth) {
            bounds.push_back(m_bounds);

            if (!IsLeaf()) {
                for (const auto& child : m_children) {
                    if (child) {
                        child->GetNodeBounds(bounds, maxDepth);
                    }
                }
            }
        }
    }

    U32 OctreeNode::GetNodeCount() const {
        U32 count = 1;
        if (!IsLeaf()) {
            for (const auto& child : m_children) {
                if (child) {
                    count += child->GetNodeCount();
                }
            }
        }
        return count;
    }

    U32 OctreeNode::GetMaxDepth() const {
        if (IsLeaf()) {
            return m_depth;
        }

        U32 maxChildDepth = m_depth;
        for (const auto& child : m_children) {
            if (child) {
                maxChildDepth = std::max(maxChildDepth, child->GetMaxDepth());
            }
        }
        return maxChildDepth;
    }

    void OctreeNode::InsertIntoChildren(Entity* entity) {
        Math::BoundingBox entityBounds = GetEntityBounds(entity);

        // Check which octants the entity overlaps
        for (U32 i = 0; i < 8; ++i) {
            if (m_children[i]) {
                if (m_children[i]->GetBounds().Intersects(entityBounds)) {
                    m_children[i]->InsertEntity(entity);
                }
            }
        }
    }

    void OctreeNode::CollectFromChildren() {
        m_entities.clear();
        for (auto& child : m_children) {
            if (child) {
                child->GetAllEntities(m_entities);
            }
        }
    }

    bool OctreeNode::FitsInOctant(Entity* entity, Octant octant) const {
        if (!m_children[octant]) return false;

        Math::BoundingBox entityBounds = GetEntityBounds(entity);
        return m_children[octant]->GetBounds().Contains(entityBounds);
    }

    Math::BoundingBox OctreeNode::GetEntityBounds(Entity* entity) const {
        if (auto* renderer = entity->GetComponent<MeshRenderer>()) {
            return renderer->GetBounds();
        }

        // Fallback to point bounds
        Math::Vector3 pos = entity->GetTransform().GetWorldPosition();
        return Math::BoundingBox(pos - Math::Vector3(0.5f), pos + Math::Vector3(0.5f));
    }

    // ================== Octree Implementation ==================

    Octree::Octree(const Config& config)
        : m_config(config) {
        if (m_config.worldBounds.IsValid()) {
            m_root = CreateScope<OctreeNode>(m_config.worldBounds, 0);
        }
    }

    void Octree::Insert(Entity* entity) {
        if (!entity) return;

        // Ensure we have a root
        if (!m_root) {
            Math::Vector3 pos = entity->GetTransform().GetWorldPosition();
            m_config.worldBounds = Math::BoundingBox(
                pos - Math::Vector3(100.0f),
                pos + Math::Vector3(100.0f)
            );
            m_root = CreateScope<OctreeNode>(m_config.worldBounds, 0);
        }

        // Expand if needed
        if (m_config.dynamicExpansion) {
            if (auto* renderer = entity->GetComponent<MeshRenderer>()) {
                Math::BoundingBox entityBounds = renderer->GetBounds();
                if (!m_root->GetBounds().Contains(entityBounds)) {
                    ExpandToInclude(entityBounds);
                }
            }
        }

        // Find best node and insert
        OctreeNode* bestNode = FindBestNode(entity, m_root.get());
        InsertIntoNode(entity, bestNode);
    }

    void Octree::Remove(Entity* entity) {
        if (!entity) return;

        auto it = m_entityToNode.find(entity);
        if (it != m_entityToNode.end()) {
            RemoveFromNode(entity, it->second);
        }
    }

    void Octree::Update(Entity* entity) {
        if (!entity) return;

        if (m_batchMode) {
            m_pendingUpdates.insert(entity);
        }
        else {
            Remove(entity);
            Insert(entity);
        }
    }

    void Octree::Clear() {
        m_root.reset();
        m_entityToNode.clear();
        m_pendingUpdates.clear();
    }

    void Octree::InsertMultiple(const std::vector<Entity*>& entities) {
        BeginBatchUpdate();
        for (Entity* entity : entities) {
            Insert(entity);
        }
        EndBatchUpdate();
    }

    void Octree::RemoveMultiple(const std::vector<Entity*>& entities) {
        BeginBatchUpdate();
        for (Entity* entity : entities) {
            Remove(entity);
        }
        EndBatchUpdate();
    }

    void Octree::UpdateAll() {
        std::vector<Entity*> allEntities;
        allEntities.reserve(m_entityToNode.size());

        for (const auto& pair : m_entityToNode) {
            allEntities.push_back(pair.first);
        }

        Clear();
        InsertMultiple(allEntities);
    }

    void Octree::Query(const Math::Frustum& frustum, std::vector<Entity*>& results) const {
        if (m_root) {
            m_root->GetEntitiesInFrustum(frustum, results);
        }
    }

    void Octree::Query(const Math::BoundingBox& bounds, std::vector<Entity*>& results) const {
        if (m_root) {
            m_root->GetEntitiesInBounds(bounds, results);
        }
    }

    void Octree::Query(const Math::Vector3& center, F32 radius, std::vector<Entity*>& results) const {
        if (m_root) {
            m_root->GetEntitiesInSphere(center, radius, results);
        }
    }

    void Octree::QueryRay(const Math::Ray& ray, F32 maxDistance, std::vector<Entity*>& results) const {
        if (m_root) {
            m_root->GetEntitiesAlongRay(ray, maxDistance, results);
        }
    }

    Entity* Octree::FindNearest(const Math::Vector3& point, F32 maxDistance) const {
        std::vector<Entity*> candidates;
        Query(point, maxDistance, candidates);

        Entity* nearest = nullptr;
        F32 nearestDistSq = maxDistance * maxDistance;

        for (Entity* entity : candidates) {
            Math::Vector3 pos = entity->GetTransform().GetWorldPosition();
            F32 distSq = (pos - point).LengthSquared();
            if (distSq < nearestDistSq) {
                nearest = entity;
                nearestDistSq = distSq;
            }
        }

        return nearest;
    }

    void Octree::FindKNearest(const Math::Vector3& point, U32 k, std::vector<Entity*>& results) const {
        // Simple implementation - could be optimized with a priority queue
        std::vector<std::pair<F32, Entity*>> distances;

        if (m_root) {
            std::vector<Entity*> allEntities;
            m_root->GetAllEntities(allEntities);

            for (Entity* entity : allEntities) {
                Math::Vector3 pos = entity->GetTransform().GetWorldPosition();
                F32 distSq = (pos - point).LengthSquared();
                distances.push_back({ distSq, entity });
            }
        }

        // Sort by distance
        std::sort(distances.begin(), distances.end());

        // Take first k
        results.clear();
        for (U32 i = 0; i < std::min(k, static_cast<U32>(distances.size())); ++i) {
            results.push_back(distances[i].second);
        }
    }

    void Octree::Optimize() {
        if (!m_root) return;

        // Collapse empty nodes
        CollapseEmptyNodes(m_root.get());

        // Could add more optimizations here
        // - Rebalance tree
        // - Merge underfilled nodes
        // - Adjust node boundaries
    }

    void Octree::Rebuild() {
        std::vector<Entity*> allEntities;
        allEntities.reserve(m_entityToNode.size());

        for (const auto& pair : m_entityToNode) {
            allEntities.push_back(pair.first);
        }

        Clear();

        // Recalculate bounds
        if (!allEntities.empty()) {
            Math::BoundingBox newBounds;
            for (Entity* entity : allEntities) {
                if (auto* renderer = entity->GetComponent<MeshRenderer>()) {
                    newBounds.ExpandToInclude(renderer->GetBounds());
                }
            }

            // Add padding
            newBounds.Scale(1.2f);
            m_config.worldBounds = newBounds;
            m_root = CreateScope<OctreeNode>(m_config.worldBounds, 0);
        }

        InsertMultiple(allEntities);
    }

    void Octree::ExpandToInclude(const Math::BoundingBox& bounds) {
        if (!m_root || m_root->GetBounds().Contains(bounds)) {
            return;
        }

        // Calculate new bounds
        Math::BoundingBox newBounds = Math::BoundingBox::Union(m_root->GetBounds(), bounds);
        newBounds.Scale(m_config.expansionFactor);

        // Store all current entities
        std::vector<Entity*> allEntities;
        allEntities.reserve(m_entityToNode.size());
        for (const auto& pair : m_entityToNode) {
            allEntities.push_back(pair.first);
        }

        // Create new root with expanded bounds
        Clear();
        m_config.worldBounds = newBounds;
        m_root = CreateScope<OctreeNode>(m_config.worldBounds, 0);

        // Re-insert all entities
        InsertMultiple(allEntities);
    }

    Octree::Statistics Octree::GetStatistics() const {
        Statistics stats;

        if (!m_root) {
            return stats;
        }

        stats.totalEntities = static_cast<U32>(m_entityToNode.size());
        stats.totalNodes = m_root->GetNodeCount();
        stats.maxDepth = m_root->GetMaxDepth();

        // Count leaf nodes
        U32 leafCount = 0;
        U32 emptyLeafCount = 0;
        U32 totalEntitiesInLeaves = 0;

        TraverseNodes([&](const OctreeNode* node) {
            if (node->IsLeaf()) {
                leafCount++;
                U32 entityCount = static_cast<U32>(node->GetEntityCount());
                totalEntitiesInLeaves += entityCount;
                if (entityCount == 0) {
                    emptyLeafCount++;
                }
            }
            });

        stats.leafNodes = leafCount;
        stats.emptyLeafNodes = emptyLeafCount;
        stats.averageEntitiesPerLeaf = leafCount > 0 ?
            static_cast<F32>(totalEntitiesInLeaves) / leafCount : 0.0f;

        return stats;
    }

    void Octree::GetAllNodeBounds(std::vector<Math::BoundingBox>& bounds, U32 maxDepth) const {
        if (m_root) {
            m_root->GetNodeBounds(bounds, maxDepth);
        }
    }

    void Octree::RemoveFromNode(Entity* entity, OctreeNode* node) {
        if (!entity || !node) return;

        node->RemoveEntity(entity);
        m_entityToNode.erase(entity);

        // Collapse empty nodes up the tree
        CollapseEmptyNodes(node);
    }

    void Octree::InsertIntoNode(Entity* entity, OctreeNode* node) {
        if (!entity || !node) return;

        node->InsertEntity(entity);
        m_entityToNode[entity] = node;
    }

    OctreeNode* Octree::FindBestNode(Entity* entity, OctreeNode* start) const {
        if (!entity || !start) return nullptr;

        // Get entity bounds
        Math::BoundingBox entityBounds;
        if (auto* renderer = entity->GetComponent<MeshRenderer>()) {
            entityBounds = renderer->GetBounds();
        }
        else {
            Math::Vector3 pos = entity->GetTransform().GetWorldPosition();
            entityBounds = Math::BoundingBox(pos - Math::Vector3(0.1f), pos + Math::Vector3(0.1f));
        }

        // Find deepest node that fully contains the entity
        OctreeNode* current = start;
        while (!current->IsLeaf()) {
            bool foundChild = false;

            // Check if entity fits entirely in one child
            for (U32 i = 0; i < 8; ++i) {
                if (auto* child = current->GetChild(static_cast<OctreeNode::Octant>(i))) {
                    if (child->GetBounds().Contains(entityBounds)) {
                        current = child;
                        foundChild = true;
                        break;
                    }
                }
            }

            if (!foundChild) {
                break; // Entity spans multiple children, stay at current level
            }
        }

        return current;
    }

    void Octree::CollapseEmptyNodes(OctreeNode* node) {
        if (!node || node->IsLeaf()) return;

        // Check if all children are empty leaves
        bool allChildrenEmpty = true;
        for (U32 i = 0; i < 8; ++i) {
            auto* child = node->GetChild(static_cast<OctreeNode::Octant>(i));
            if (child && (!child->IsLeaf() || child->GetEntityCount() > 0)) {
                allChildrenEmpty = false;
                break;
            }
        }

        if (allChildrenEmpty) {
            node->Collapse();
        }
    }

    void Octree::EndBatchUpdate() {
        m_batchMode = false;

        // Process all pending updates
        for (Entity* entity : m_pendingUpdates) {
            Remove(entity);
            Insert(entity);
        }
        m_pendingUpdates.clear();

        // Optimize after batch
        Optimize();
    }

} // namespace Angaraka::Scene