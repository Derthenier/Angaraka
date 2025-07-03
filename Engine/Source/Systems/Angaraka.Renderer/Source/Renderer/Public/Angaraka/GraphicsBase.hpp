#pragma once

#ifndef ANGARAKA_RENDERER_GRAPHICS_BASE_HPP
#define ANGARAKA_RENDERER_GRAPHICS_BASE_HPP

// Define a macro for safer COM object release if not using ComPtr everywhere
#ifndef RELEASE_COM_PTR
#define RELEASE_COM_PTR(x) { if(x){ x->Release(); x = nullptr; } }
#endif

#include <d3d12.h> // Core DX12 header
#include <d3dx12.h> // Core DX12 header
#include <dxgi1_6.h> // DXGI for adapter enumeration and swap chain
#include <wrl/client.h>     // For Microsoft::WRL::ComPtr
#include <DirectXMath.h>

#include <Angaraka/Base.hpp>

import Angaraka.Math;
import Angaraka.Math.Vector3;
import Angaraka.Math.Vector4;
import Angaraka.Math.DirectXInterop;

namespace Angaraka::Graphics::DirectX12 {

    // Define the structure of a single vertex
    struct Vertex {
        DirectX::XMFLOAT3 Position; // X, Y, Z coordinates
        DirectX::XMFLOAT4 Color;    // RGBA color
        DirectX::XMFLOAT2 UV;       // UV 
    };

    // ==================== Constant Buffer Helpers ====================

    /**
     * @brief Aligned structure for MVP constant buffer
     * Ensures proper 16-byte alignment for GPU
     */
    struct alignas(16) MVPConstantBuffer {
        DirectX::XMFLOAT4X4 model;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4X4 mvp;  // Pre-multiplied for efficiency

        void Update(const Math::Matrix4x4& modelMatrix,
            const Math::Matrix4x4& viewMatrix,
            const Math::Matrix4x4& projMatrix) {
            // Convert to DirectX format
            DirectX::XMMATRIX m = Math::MathConversion::ToDirectXMatrix(modelMatrix);
            DirectX::XMMATRIX v = Math::MathConversion::ToDirectXMatrix(viewMatrix);
            DirectX::XMMATRIX p = Math::MathConversion::ToDirectXMatrix(projMatrix);

            // Store individual matrices
            DirectX::XMStoreFloat4x4(&model, m);
            DirectX::XMStoreFloat4x4(&view, v);
            DirectX::XMStoreFloat4x4(&projection, p);

            // Pre-multiply MVP
            DirectX::XMMATRIX mvpMatrix = m * v * p;
            DirectX::XMStoreFloat4x4(&mvp, mvpMatrix);
        }
    };

    /**
     * @brief Aligned structure for per-object constant buffer
     */
    struct alignas(16) PerObjectConstantBuffer {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 worldInverseTranspose;  // For normal transformation
        DirectX::XMFLOAT4 color;
        DirectX::XMFLOAT3 padding;  // Ensure 16-byte alignment
        float metalness;
        float roughness;
        float ao;
        float _padding[2];  // Pad to 16-byte boundary

        void Update(const Math::Matrix4x4& worldMatrix, const Math::Vector4& objectColor = Math::Vector4(1, 1, 1, 1)) {
            DirectX::XMMATRIX w = Math::MathConversion::ToDirectXMatrix(worldMatrix);
            DirectX::XMStoreFloat4x4(&world, w);

            // Calculate world inverse transpose for normal transformation
            DirectX::XMMATRIX wit = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, w));
            DirectX::XMStoreFloat4x4(&worldInverseTranspose, wit);

            color = Math::MathConversion::ToDirectXFloat4(objectColor);
        }
    };
}

#ifdef _DEBUG

#ifndef DXCall
#define DXCall(x)                                                               \
if (FAILED(x)) {                                                                \
    char line_number[32];                                                       \
    sprintf_s(line_number, "%u", __LINE__);                                     \
    OutputDebugStringA("Error in: ");                                           \
    OutputDebugStringA(__FILE__);                                               \
    OutputDebugStringA("\nLine: ");                                             \
    OutputDebugStringA(line_number);                                            \
    OutputDebugStringA("\n");                                                   \
    OutputDebugStringA(#x);                                                     \
    OutputDebugStringA("\n");                                                   \
    char file_name[192];                                                        \
    sprintf_s(file_name, "%s", __FILE__);                                       \
    AGK_ERROR("Error in: \"{0}\" Line: {1}", file_name, line_number);           \
    __debugbreak();                                                             \
}
#endif // !DXCall

#ifndef NAME_D3D12_OBJECT
#define NAME_D3D12_OBJECT(obj, name)                        \
{                                                           \
    obj->SetName(L""#name);                                   \
    OutputDebugStringA("::D3D12 Object Created: ");         \
    OutputDebugStringA(name);                               \
    OutputDebugStringA("\n");                               \
    AGK_INFO("     ::D3D12 Object Created: {0}", name);     \
}
#endif // !NAME_D3D12_OBJECT

#ifndef NAME_D3D12_OBJECT_INDEXED
#define NAME_D3D12_OBJECT_INDEXED(obj, n, name)                      \
{                                                                    \
wchar_t full_nameW[128];                                             \
if (swprintf_s(full_nameW, L"%s[%llu]", L""#name, (uint64_t)n) > 0 ) { \
    obj->SetName(full_nameW);                                        \
    char full_name[128];                                             \
    sprintf_s(full_name, "%s[%llu]", name, (uint64_t)n);             \
    OutputDebugStringA("::D3D12 Object Created: ");                  \
    OutputDebugStringA(full_name);                                   \
    OutputDebugStringA("\n");                                        \
    AGK_INFO("     ::D3D12 Object Created: {0}", full_name);         \
}}
#endif // !NAME_D3D12_OBJECT_INDEXED


#else

#ifndef DXCall
#define DXCall(x) x
#endif // !DXCall

#ifndef NAME_D3D12_OBJECT
#define NAME_D3D12_OBJECT(obj, name)
#endif // !NAME_D3D12_OBJECT

#ifndef NAME_D3D12_OBJECT_INDEXED
#define NAME_D3D12_OBJECT_INDEXED(obj, i, name)
#endif // !NAME_D3D12_OBJECT_INDEXED

#endif

#endif // ANGARAKA_RENDERER_GRAPHICS_BASE_HPP