module;

#include "Angaraka/Base.hpp"
#include "Angaraka/MathCore.hpp"

export module Angaraka.Scene.Components.MeshRenderer;

import Angaraka.Scene.Transform;
import Angaraka.Scene.Component;
import Angaraka.Scene.Entity;
import Angaraka.Core.Resources;

namespace Angaraka::SceneSystem {

    /**
     * @brief Component that renders a mesh at the entity's position
     *
     * This component integrates with the existing DirectX12GraphicsSystem
     * to render meshes using the entity's transform.
     */
    export class MeshRenderer : public ComponentBase<MeshRenderer> {
    public:
        MeshRenderer() = default;
        ~MeshRenderer() override = default;

        // ================== Mesh Management ==================

        /**
         * @brief Set the mesh to render by resource ID
         * @param meshResourceId ID of the mesh resource (e.g., "props/barrel")
         */
        void SetMesh(const String& meshResourceId);

        /**
         * @brief Get the current mesh resource ID
         */
        const String& GetMeshResourceId() const;

        /**
         * @brief Get the loaded mesh resource
         * @return Mesh resource or nullptr if not loaded
         */
        Core::Resource* GetMeshResource() const;

        // ================== Rendering Properties ==================

        /**
         * @brief Set whether this mesh casts shadows
         */
        void SetCastShadows(bool cast);

        bool GetCastShadows() const;

        /**
         * @brief Set whether this mesh receives shadows
         */
        void SetReceiveShadows(bool receive);

        bool GetReceiveShadows() const;

        /**
         * @brief Set render layer for sorting
         */
        void SetRenderLayer(U32 layer);

        U32 GetRenderLayer() const;

        // ================== Bounds ==================

        /**
         * @brief Get world-space bounding box
         * @return Bounding box in world space
         */
        const Math::BoundingBox& GetBounds() const;

        /**
         * @brief Check if mesh is visible in frustum
         * @param frustum Camera frustum to test against
         * @return True if visible
         */
        bool IsVisibleInFrustum(const Math::Frustum& frustum) const;

        // ================== Component Lifecycle ==================

        void OnEnable() override;

        void OnDisable() override;

        void OnTransformChanged() override;

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
        void UpdateBounds() const;
    };
}