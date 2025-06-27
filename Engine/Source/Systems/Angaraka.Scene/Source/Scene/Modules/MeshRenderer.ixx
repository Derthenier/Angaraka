module;

#include "Angaraka/Base.hpp"
#include "Angaraka/MathCore.hpp"

export module Angaraka.Scene.Components.MeshRenderer;

import Angaraka.Scene.Transform;
import Angaraka.Scene.Component;
import Angaraka.Scene.Entity;
import Angaraka.Core.Resources;
import Angaraka.Scene;

namespace Angaraka::SceneSystem {

    /**
     * @brief Component that renders a mesh at the entity's position
     *
     * This component integrates with the existing DirectX12GraphicsSystem
     * to render meshes using the entity's transform.
     */
    export class MeshRenderer : public ComponentBase<MeshRenderer> {
    public:
        inline MeshRenderer() = default;
        inline ~MeshRenderer() override = default;

        // ================== Mesh Management ==================

        /**
         * @brief Set the mesh to render by resource ID
         * @param meshResourceId ID of the mesh resource (e.g., "props/barrel")
         */
        inline void SetMesh(const String& meshResourceId) {
            if (m_meshResourceId != meshResourceId) {
                m_meshResourceId = meshResourceId;
                m_meshResource = nullptr; // Clear cached resource
                m_boundsDirty = true;
            }
        }

        /**
         * @brief Get the current mesh resource ID
         */
        inline const String& GetMeshResourceId() const {
            return m_meshResourceId;
        }

        /**
         * @brief Get the loaded mesh resource
         * @return Mesh resource or nullptr if not loaded
         */
        inline Core::Resource* GetMeshResource() const {
            if (!m_meshResource && !m_meshResourceId.empty()) {
                // Lazy load the mesh resource
                if (auto* scene = GetScene()) {
                    if (auto* resourceManager = scene->GetResourceManager()) {
                        // Note: This is a simplified version - you may need to adjust
                        // based on your actual ResourceManager API
                        m_meshResource = resourceManager->GetResource<Core::Resource>(m_meshResourceId);
                    }
                }
            }
            return m_meshResource.get();
        }

        // ================== Rendering Properties ==================

        /**
         * @brief Set whether this mesh casts shadows
         */
        inline void SetCastShadows(bool cast) {
            m_castShadows = cast;
        }

        inline bool GetCastShadows() const {
            return m_castShadows;
        }

        /**
         * @brief Set whether this mesh receives shadows
         */
        inline void SetReceiveShadows(bool receive) {
            m_receiveShadows = receive;
        }

        inline bool GetReceiveShadows() const {
            return m_receiveShadows;
        }

        /**
         * @brief Set render layer for sorting
         */
        inline void SetRenderLayer(U32 layer) {
            m_renderLayer = layer;
        }

        inline U32 GetRenderLayer() const {
            return m_renderLayer;
        }

        // ================== Bounds ==================

        /**
         * @brief Get world-space bounding box
         * @return Bounding box in world space
         */
        inline const Math::BoundingBox& GetBounds() const {
            if (m_boundsDirty) {
                UpdateBounds();
            }
            return m_worldBounds;
        }

        /**
         * @brief Check if mesh is visible in frustum
         * @param frustum Camera frustum to test against
         * @return True if visible
         */
        inline bool IsVisibleInFrustum(const Math::Frustum& frustum) const {
            return frustum.Intersects(GetBounds());
        }

        // ================== Component Lifecycle ==================

        inline void OnEnable() override {
            // Register with rendering system if needed
            AGK_TRACE("MeshRenderer: Enabled for entity '{}'", GetEntity()->GetName());
        }

        inline void OnDisable() override {
            // Unregister from rendering system if needed
            AGK_TRACE("MeshRenderer: Disabled for entity '{}'", GetEntity()->GetName());
        }

        inline void OnTransformChanged() override {
            // Mark bounds as dirty when transform changes
            m_boundsDirty = true;
        }

    private:
        // Mesh data
        String m_meshResourceId;
        mutable Reference<Core::Resource> m_meshResource; // Cached resource

        // Rendering properties
        bool m_castShadows = true;
        bool m_receiveShadows = true;
        U32 m_renderLayer = 0; // 0 = default layer

        // Bounds
        mutable Math::BoundingBox m_worldBounds;
        mutable bool m_boundsDirty = true;

        /**
         * @brief Update world bounds from mesh and transform
         */
        inline void UpdateBounds() const {
            // Default bounds if no mesh
            Math::BoundingBox localBounds(
                Math::Vector3(-0.5f, -0.5f, -0.5f),
                Math::Vector3(0.5f, 0.5f, 0.5f)
            );

            // TODO: Get actual bounds from mesh resource
            // For now, use default unit cube bounds

            // Transform local bounds to world space
            auto& transform = GetTransform();
            Math::Matrix4x4 worldMatrix = transform.GetWorldMatrix();

            // Transform all 8 corners of the bounding box
            Math::Vector3 corners[8] = {
                Math::Vector3(localBounds.min.x, localBounds.min.y, localBounds.min.z),
                Math::Vector3(localBounds.max.x, localBounds.min.y, localBounds.min.z),
                Math::Vector3(localBounds.min.x, localBounds.max.y, localBounds.min.z),
                Math::Vector3(localBounds.max.x, localBounds.max.y, localBounds.min.z),
                Math::Vector3(localBounds.min.x, localBounds.min.y, localBounds.max.z),
                Math::Vector3(localBounds.max.x, localBounds.min.y, localBounds.max.z),
                Math::Vector3(localBounds.min.x, localBounds.max.y, localBounds.max.z),
                Math::Vector3(localBounds.max.x, localBounds.max.y, localBounds.max.z)
            };

            // Transform first corner
            Math::Vector3 worldMin = worldMatrix.TransformPoint(corners[0]);
            Math::Vector3 worldMax = worldMin;

            // Transform remaining corners and expand bounds
            for (int i = 1; i < 8; ++i) {
                Math::Vector3 worldCorner = worldMatrix.TransformPoint(corners[i]);
                worldMin = Math::Vector3::Min(worldMin, worldCorner);
                worldMax = Math::Vector3::Max(worldMax, worldCorner);
            }

            m_worldBounds = Math::BoundingBox(worldMin, worldMax);
            m_boundsDirty = false;
        }
    };
}