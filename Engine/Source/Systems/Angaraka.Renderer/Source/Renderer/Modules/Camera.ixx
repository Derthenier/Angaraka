// Angaraka.Camera.ixx
// Module for managing the camera (view and projection matrices).
module;

#include "Angaraka/GraphicsBase.hpp"

export module Angaraka.Camera;

import Angaraka.Math;
import Angaraka.Math.Vector3;
import Angaraka.Math.Matrix4x4;
import Angaraka.Math.DirectXInterop;

namespace Angaraka {

    // Define movement directions for the camera
    export enum class Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,    // For flying camera (vertical movement)
        DOWN   // For flying camera (vertical movement)
    };

    export enum class Camera_Yaw_Pitch {
        YAW,
        PITCH
    };

    export class Camera {
    public:
        Camera();
        ~Camera();

        // [Previous methods remain the same]
        void Initialize(F32 fovY, F32 aspectRatio, F32 nearZ, F32 farZ);
        void ProcessMouseMovement(F32 xOffset, F32 yOffset, bool constrainPitch = true);
        void SetMoveState(Camera_Movement direction, bool active);
        void SetLens(F32 fovY, F32 aspectRatio, F32 nearZ, F32 farZ);
        void Update(F32 deltaTime);
        void Reset();

        // ==================== Math Library Accessors ====================
        // These return Angaraka.Math types for game logic
        Math::Matrix4x4 GetViewMatrix() const;
        Math::Matrix4x4 GetProjectionMatrix() const;
        const Math::Vector3& GetPosition() const { return m_position; }
        const Math::Vector3& GetFront() const { return m_front; }
        const Math::Vector3& GetUp() const { return m_up; }
        const Math::Vector3& GetRight() const { return m_right; }

        // ==================== DirectX Conversion Methods ====================
        // These provide DirectX types for rendering, avoiding conversion overhead

        /**
         * @brief Get view matrix in DirectX format (cached)
         */
        DirectX::XMMATRIX GetViewMatrixDX() const {
            if (m_viewDirty) {
                UpdateViewMatrix();
            }
            return m_cachedViewMatrixDX;
        }

        /**
         * @brief Get projection matrix in DirectX format (cached)
         */
        DirectX::XMMATRIX GetProjectionMatrixDX() const {
            if (m_projectionDirty) {
                UpdateProjectionMatrix();
            }
            return m_cachedProjectionMatrixDX;
        }

        /**
         * @brief Get view-projection matrix pre-multiplied (common GPU operation)
         */
        DirectX::XMMATRIX GetViewProjectionMatrixDX() const {
            return GetViewMatrixDX() * GetProjectionMatrixDX();
        }

        /**
         * @brief Fill a constant buffer with camera matrices
         * @param cb Constant buffer to fill
         * @param modelMatrix World transform of the object being rendered
         */
        void FillMVPConstantBuffer(Graphics::DirectX12::MVPConstantBuffer& cb, const Math::Matrix4x4& modelMatrix) const {
            cb.Update(modelMatrix, GetViewMatrix(), GetProjectionMatrix());
        }

        /**
         * @brief Get camera data for structured buffer (GPU instancing)
         */
        struct CameraData {
            DirectX::XMFLOAT4X4 view;
            DirectX::XMFLOAT4X4 projection;
            DirectX::XMFLOAT4X4 viewProjection;
            DirectX::XMFLOAT3 position;
            float nearPlane;
            DirectX::XMFLOAT3 forward;
            float farPlane;
            DirectX::XMFLOAT3 right;
            float fov;
            DirectX::XMFLOAT3 up;
            float aspectRatio;
        };

        CameraData GetCameraData() const {
            CameraData data;

            // Matrices
            DirectX::XMStoreFloat4x4(&data.view, GetViewMatrixDX());
            DirectX::XMStoreFloat4x4(&data.projection, GetProjectionMatrixDX());
            DirectX::XMStoreFloat4x4(&data.viewProjection, GetViewProjectionMatrixDX());

            // Vectors
            data.position = Math::MathConversion::ToDirectXFloat3(m_position);
            data.forward = Math::MathConversion::ToDirectXFloat3(m_front);
            data.right = Math::MathConversion::ToDirectXFloat3(m_right);
            data.up = Math::MathConversion::ToDirectXFloat3(m_up);

            // Scalars
            data.nearPlane = m_nearZ;
            data.farPlane = m_farZ;
            data.fov = m_zoom;
            data.aspectRatio = m_aspectRatio;

            return data;
        }

        // ==================== Frustum Culling Support ====================

        /**
         * @brief Get frustum planes for GPU culling
         */
        struct FrustumPlanes {
            DirectX::XMFLOAT4 planes[6];  // Left, Right, Bottom, Top, Near, Far
        };

        FrustumPlanes GetFrustumPlanes() const {
            FrustumPlanes frustum;
            DirectX::XMMATRIX vpMatrix = GetViewProjectionMatrixDX();

            // Extract frustum planes from view-projection matrix
            // This is a common technique for frustum culling
            DirectX::XMFLOAT4X4 vp;
            DirectX::XMStoreFloat4x4(&vp, vpMatrix);

            // Left plane
            frustum.planes[0] = DirectX::XMFLOAT4(
                vp._14 + vp._11, vp._24 + vp._21, vp._34 + vp._31, vp._44 + vp._41
            );

            // Right plane
            frustum.planes[1] = DirectX::XMFLOAT4(
                vp._14 - vp._11, vp._24 - vp._21, vp._34 - vp._31, vp._44 - vp._41
            );

            // Bottom plane
            frustum.planes[2] = DirectX::XMFLOAT4(
                vp._14 + vp._12, vp._24 + vp._22, vp._34 + vp._32, vp._44 + vp._42
            );

            // Top plane
            frustum.planes[3] = DirectX::XMFLOAT4(
                vp._14 - vp._12, vp._24 - vp._22, vp._34 - vp._32, vp._44 - vp._42
            );

            // Near plane
            frustum.planes[4] = DirectX::XMFLOAT4(
                vp._13, vp._23, vp._33, vp._43
            );

            // Far plane
            frustum.planes[5] = DirectX::XMFLOAT4(
                vp._14 - vp._13, vp._24 - vp._23, vp._34 - vp._33, vp._44 - vp._43
            );

            // Normalize planes
            for (int i = 0; i < 6; ++i) {
                DirectX::XMVECTOR plane = DirectX::XMLoadFloat4(&frustum.planes[i]);
                plane = DirectX::XMPlaneNormalize(plane);
                DirectX::XMStoreFloat4(&frustum.planes[i], plane);
            }

            return frustum;
        }

    private:
        // [Previous member variables remain the same]
        Math::Vector3 m_position;
        Math::Vector3 m_front;
        Math::Vector3 m_up;
        Math::Vector3 m_right;
        Math::Vector3 m_worldUp;
        F32 m_yaw;
        F32 m_pitch;
        F32 m_movementSpeed;
        F32 m_mouseSensitivity;
        F32 m_zoom;
        F32 m_fov;
        F32 m_aspectRatio;
        F32 m_nearZ;
        F32 m_farZ;
        bool m_isMovingForward;
        bool m_isMovingBackward;
        bool m_isMovingLeft;
        bool m_isMovingRight;
        bool m_isMovingUp;
        bool m_isMovingDown;

        // ==================== Caching for Performance ====================
        // These avoid repeated conversions every frame
        mutable bool m_viewDirty = true;
        mutable bool m_projectionDirty = true;
        mutable DirectX::XMMATRIX m_cachedViewMatrixDX;
        mutable DirectX::XMMATRIX m_cachedProjectionMatrixDX;

        void UpdateCameraVectors();

        // Cache update methods
        void UpdateViewMatrix() const;
        void UpdateProjectionMatrix() const;

        // Mark caches as dirty when camera changes
        void InvalidateViewCache();
        void InvalidateProjectionCache();
    };

} // namespace Angaraka