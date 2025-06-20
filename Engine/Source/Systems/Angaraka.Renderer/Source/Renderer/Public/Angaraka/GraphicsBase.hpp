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

namespace Angaraka::Graphics::DirectX12 {

    // Define the structure of a single vertex
    struct Vertex {
        DirectX::XMFLOAT3 Position; // X, Y, Z coordinates
        DirectX::XMFLOAT4 Color;    // RGBA color
        DirectX::XMFLOAT2 UV;       // UV 
    };

    // Must be 256-byte aligned for D3D12 constant buffers
    // Use __declspec(align(256)) for this
    struct ModelViewProjectionConstantBuffer {
        DirectX::XMMATRIX model;        // World transformation
        DirectX::XMMATRIX view;         // Camera transformation (will add in a later sprint)
        DirectX::XMMATRIX projection;   // Projection transformation (will add in a later sprint)
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
    obj->SetName(L#name);                                   \
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
if (swprintf_s(full_nameW, L"%s[%llu]", L#name, (uint64_t)n) > 0 ) { \
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