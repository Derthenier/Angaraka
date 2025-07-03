module;

#include <Angaraka/Base.hpp>
#include <DirectXMath.h>

export module Angaraka.Math.DirectXInterop;

import Angaraka.Math.Vector3;
import Angaraka.Math.Vector4;
import Angaraka.Math.Matrix4x4;

namespace Angaraka::Math {

    /**
     * @brief Utility functions for converting between Angaraka Math and DirectX Math
     *
     * These functions bridge the gap between our engine's math library and
     * DirectX's required matrix format for GPU operations.
     */
    export class MathConversion
    {
    public:
        /**
         * @brief Convert Angaraka Matrix4x4 to DirectX XMMATRIX
         * @param matrix Angaraka matrix to convert
         * @return DirectX matrix for GPU operations
         */
        inline static DirectX::XMMATRIX ToDirectXMatrix(const Matrix4x4& matrix)
        {
            // Angaraka Math uses row-major, DirectX uses column-major
            // Need to transpose when converting
            return DirectX::XMMATRIX(
                matrix(0, 0), matrix(1, 0), matrix(2, 0), matrix(3, 0),  // First column
                matrix(0, 1), matrix(1, 1), matrix(2, 1), matrix(3, 1),  // Second column
                matrix(0, 2), matrix(1, 2), matrix(2, 2), matrix(3, 2),  // Third column
                matrix(0, 3), matrix(1, 3), matrix(2, 3), matrix(3, 3)   // Fourth column
            );
        }

        /**
         * @brief Convert Angaraka Vector3 to DirectX XMFLOAT3
         * @param vector Angaraka vector to convert
         * @return DirectX float3 structure
         */
        inline static DirectX::XMFLOAT3 ToDirectXFloat3(const Vector3& vector)
        {
            return DirectX::XMFLOAT3(vector.x, vector.y, vector.z);
        }

        /**
         * @brief Convert Angaraka Vector4 to DirectX XMFLOAT4
         * @param vector Angaraka vector to convert
         * @return DirectX float4 structure
         */
        inline static DirectX::XMFLOAT4 ToDirectXFloat4(const Vector4& vector)
        {
            return DirectX::XMFLOAT4(vector.x, vector.y, vector.z, vector.w);
        }

        /**
         * @brief Convert DirectX XMMATRIX to Angaraka Matrix4x4
         * @param matrix DirectX matrix to convert
         * @return Angaraka matrix
         */
        inline static Matrix4x4 FromDirectXMatrix(const DirectX::XMMATRIX& matrix)
        {
            // DirectX uses column-major, Angaraka Math uses row-major
            // Need to transpose when converting
            DirectX::XMFLOAT4X4 floatMatrix;
            DirectX::XMStoreFloat4x4(&floatMatrix, matrix);

            return Matrix4x4(
                floatMatrix._11, floatMatrix._21, floatMatrix._31, floatMatrix._41,
                floatMatrix._12, floatMatrix._22, floatMatrix._32, floatMatrix._42,
                floatMatrix._13, floatMatrix._23, floatMatrix._33, floatMatrix._43,
                floatMatrix._14, floatMatrix._24, floatMatrix._34, floatMatrix._44
            );
        }

        /**
         * @brief Convert DirectX XMFLOAT3 to Angaraka Vector3
         * @param vector DirectX float3 to convert
         * @return Angaraka vector
         */
        inline static Vector3 FromDirectXFloat3(const DirectX::XMFLOAT3& vector)
        {
            return Vector3(vector.x, vector.y, vector.z);
        }
    };
}