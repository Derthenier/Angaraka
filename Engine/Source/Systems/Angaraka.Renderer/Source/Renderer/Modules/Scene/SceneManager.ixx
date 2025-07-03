module;

#include "Angaraka/GraphicsBase.hpp"

export module Angaraka.Graphics.DirectX12.SceneManager;

import Angaraka.Math;
import Angaraka.Math.Vector3;
import Angaraka.Math.Matrix4x4;
import Angaraka.Graphics.DirectX12;
import Angaraka.Core.ResourceCache;

namespace Angaraka::Graphics::DirectX12::Scene {

    /**
     * @brief Represents a single object instance in the 3D scene
     *
     * Each SceneObject references a mesh resource and has its own transform.
     * This allows multiple instances of the same mesh at different positions.
     */
    export struct Object {
        // Identification
        String name;                    ///< Human-readable name for debugging
        String meshResourceId;          ///< ID of the mesh resource to render

        // Transform components
        Math::Vector3 position{ 0.0f, 0.0f, 0.0f };    ///< World position
        Math::Vector3 rotation{ 0.0f, 0.0f, 0.0f };    ///< Euler angles (radians)
        Math::Vector3 scale{ 1.0f, 1.0f, 1.0f };       ///< Scale factors

        // Rendering state
        bool visible = true;                 ///< Whether to render this object

        // Animation (optional - for future use)
        Math::Vector3 rotationSpeed{ 0.0f, 0.0f, 0.0f }; ///< Auto-rotation per second

        /**
         * @brief Calculate the world transform matrix for this object
         * @return World transform matrix combining scale, rotation, and translation
         */
        Math::Matrix4x4 GetWorldMatrix() const
        {
            // Create transformation matrices
            Math::Matrix4x4 scaleMatrix = Math::Matrix4x4::Scale(scale);
            Math::Matrix4x4 rotationMatrix = Math::Matrix4x4::RotationEuler(rotation);
            Math::Matrix4x4 translationMatrix = Math::Matrix4x4::Translation(position);

            // Combine: Scale * Rotation * Translation
            return scaleMatrix * rotationMatrix * translationMatrix;
        }

        /**
         * @brief Update object animation (call each frame)
         * @param deltaTime Time elapsed since last frame in seconds
         */
        void Update(F32 deltaTime)
        {
            // Apply automatic rotation if specified
            if (rotationSpeed.x != 0.0f || rotationSpeed.y != 0.0f || rotationSpeed.z != 0.0f)
            {
                rotation.x += rotationSpeed.x * deltaTime;
                rotation.y += rotationSpeed.y * deltaTime;
                rotation.z += rotationSpeed.z * deltaTime;

                // Keep angles in [0, 2π] range
                if (rotation.x > Math::Constants::TwoPiF) rotation.x -= Math::Constants::TwoPiF;
                if (rotation.y > Math::Constants::TwoPiF) rotation.y -= Math::Constants::TwoPiF;
                if (rotation.z > Math::Constants::TwoPiF) rotation.z -= Math::Constants::TwoPiF;
            }
        }
    };

    /**
     * @brief Manages all objects in the 3D scene
     *
     * Handles loading, updating, and providing render data for scene objects.
     * Integrates with ResourceManager and Graphics System for efficient rendering.
     */
    export class Manager {
    public:
        explicit Manager(
            Core::CachedResourceManager* resourceManager,
            DirectX12GraphicsSystem* graphicsSystem
        );
        ~Manager() = default;

        // Object management
        void AddObject(const Object& object);
        void RemoveObject(const std::string& name);
        Object* GetObject(const std::string& name);

        // Scene operations
        void LoadSceneFromBundle(const std::string& bundleName);
        void Update(F32 deltaTime);
        void Clear();

        // Rendering support
        const std::vector<Object>& GetVisibleObjects() const { return m_objects; }
        size_t GetObjectCount() const { return m_objects.size(); }

        // Utility
        void CreateTestScene(); ///< Create a demo scene with multiple objects

    private:
        // Dependencies
        Core::CachedResourceManager* m_resourceManager;
        DirectX12GraphicsSystem* m_meshManager;

        // Scene data
        std::vector<Object> m_objects;
        std::unordered_map<std::string, size_t> m_objectIndexMap; ///< Name to index mapping

        // Helper methods
        void RebuildIndexMap();
        bool IsValidMeshResource(const std::string& meshResourceId) const;
    };
}