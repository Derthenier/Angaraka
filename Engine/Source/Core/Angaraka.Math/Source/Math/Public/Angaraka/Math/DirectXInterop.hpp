// Angaraka / Math / DirectXInterop.hpp
// Centralized conversion utilities between Angaraka.Math and DirectX types
#pragma once

#include "Vector3.hpp"
#include "Vector4.hpp"
#include "Matrix44.hpp"
#include <DirectXMath.h>

namespace Angaraka::Math {

    /**
     * @brief Conversion utilities for DirectX interoperability
     *
     * Design principles:
     * 1. Zero-copy conversions where possible (reinterpret_cast for compatible layouts)
     * 2. Explicit conversions only (no implicit conversions to prevent accidents)
     * 3. Centralized in one place for easy maintenance
     * 4. SIMD-friendly for performance
     */
    class DirectXInterop {
    public:
        // ==================== Vector Conversions ====================

        /**
         * @brief Convert Vector3 to XMVECTOR (zero-copy when aligned)
         * Note: XMVECTOR is __m128, so it includes a 4th component (w=0 for directions, w=1 for positions)
         */
        static DirectX::XMVECTOR ToXMVECTOR(const Vector3& v, float w = 0.0f) {
            return DirectX::XMVectorSet(v.x, v.y, v.z, w);
        }

        /**
         * @brief Convert Vector3 to XMFLOAT3 (for vertex buffers)
         */
        static DirectX::XMFLOAT3 ToXMFLOAT3(const Vector3& v) {
            return DirectX::XMFLOAT3(v.x, v.y, v.z);
        }

        /**
         * @brief Convert Vector4 to XMVECTOR (potentially zero-copy if aligned)
         */
        static DirectX::XMVECTOR ToXMVECTOR(const Vector4& v) {
            return DirectX::XMVectorSet(v.x, v.y, v.z, v.w);
        }

        /**
         * @brief Convert Vector4 to XMFLOAT4 (for constant buffers)
         */
        static DirectX::XMFLOAT4 ToXMFLOAT4(const Vector4& v) {
            return DirectX::XMFLOAT4(v.x, v.y, v.z, v.w);
        }

        // ==================== Matrix Conversions ====================

        /**
         * @brief Convert Matrix4x4 to XMMATRIX
         * Note: Angaraka uses row-major, DirectX uses column-major, so we transpose
         */
        static DirectX::XMMATRIX ToXMMATRIX(const Matrix4x4& m) {
            // Transpose during conversion from row-major to column-major
            return DirectX::XMMATRIX(
                m(0, 0), m(1, 0), m(2, 0), m(3, 0),  // First column
                m(0, 1), m(1, 1), m(2, 1), m(3, 1),  // Second column
                m(0, 2), m(1, 2), m(2, 2), m(3, 2),  // Third column
                m(0, 3), m(1, 3), m(2, 3), m(3, 3)   // Fourth column
            );
        }

        /**
         * @brief Convert Matrix4x4 to XMFLOAT4X4 (for constant buffers)
         * This maintains the layout for GPU upload
         */
        static DirectX::XMFLOAT4X4 ToXMFLOAT4X4(const Matrix4x4& m) {
            DirectX::XMFLOAT4X4 result;
            DirectX::XMStoreFloat4x4(&result, ToXMMATRIX(m));
            return result;
        }

        // ==================== Reverse Conversions ====================

        /**
         * @brief Convert XMVECTOR to Vector3 (discards w component)
         */
        static Vector3 ToVector3(DirectX::XMVECTOR v) {
            DirectX::XMFLOAT3 f3;
            DirectX::XMStoreFloat3(&f3, v);
            return Vector3(f3.x, f3.y, f3.z);
        }

        /**
         * @brief Convert XMVECTOR to Vector4
         */
        static Vector4 ToVector4(DirectX::XMVECTOR v) {
            DirectX::XMFLOAT4 f4;
            DirectX::XMStoreFloat4(&f4, v);
            return Vector4(f4.x, f4.y, f4.z, f4.w);
        }

        /**
         * @brief Convert XMMATRIX to Matrix4x4
         * Note: Transposes from column-major back to row-major
         */
        static Matrix4x4 ToMatrix4x4(const DirectX::XMMATRIX& m) {
            DirectX::XMFLOAT4X4 f4x4;
            DirectX::XMStoreFloat4x4(&f4x4, m);

            // Transpose during conversion from column-major to row-major
            return Matrix4x4(
                f4x4._11, f4x4._21, f4x4._31, f4x4._41,
                f4x4._12, f4x4._22, f4x4._32, f4x4._42,
                f4x4._13, f4x4._23, f4x4._33, f4x4._43,
                f4x4._14, f4x4._24, f4x4._34, f4x4._44
            );
        }
    };

} // namespace Angaraka::Math