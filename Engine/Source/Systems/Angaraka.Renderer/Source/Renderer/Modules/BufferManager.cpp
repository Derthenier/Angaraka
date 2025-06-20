// BufferManager.cpp
// Implementation of Angaraka.Graphics.DirectX12.BufferManager module.

module;

// Includes for internal implementation details
#include "Angaraka/GraphicsBase.hpp"
#include <stdexcept>

module Angaraka.Graphics.DirectX12.BufferManager;

namespace {

    const Angaraka::Graphics::DirectX12::Vertex cubeVertices[] = {
        { DirectX::XMFLOAT3(-0.5f, +0.5f, -0.5f), DirectX::XMFLOAT4(+0.0f, +1.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.750f) }, // +Y (top face)
        { DirectX::XMFLOAT3(+0.5f, +0.5f, -0.5f), DirectX::XMFLOAT4(+0.0f, +1.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.250f) },
        { DirectX::XMFLOAT3(+0.5f, +0.5f, +0.5f), DirectX::XMFLOAT4(+0.0f, +1.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.250f) },
        { DirectX::XMFLOAT3(-0.5f, +0.5f, +0.5f), DirectX::XMFLOAT4(+0.0f, +1.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.750f) },

        { DirectX::XMFLOAT3(-0.5f, -0.5f, +0.5f), DirectX::XMFLOAT4(+0.0f, -1.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.750f) }, // -Y (bottom face)
        { DirectX::XMFLOAT3(+0.5f, -0.5f, +0.5f), DirectX::XMFLOAT4(+0.0f, -1.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.250f) },
        { DirectX::XMFLOAT3(+0.5f, -0.5f, -0.5f), DirectX::XMFLOAT4(+0.0f, -1.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.250f) },
        { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT4(+0.0f, -1.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.750f) },

        { DirectX::XMFLOAT3(+0.5f, +0.5f, +0.5f), DirectX::XMFLOAT4(+1.0f, +0.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.250f) }, // +X (right face)
        { DirectX::XMFLOAT3(+0.5f, +0.5f, -0.5f), DirectX::XMFLOAT4(+1.0f, +0.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.250f) },
        { DirectX::XMFLOAT3(+0.5f, -0.5f, -0.5f), DirectX::XMFLOAT4(+1.0f, +0.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.750f) },
        { DirectX::XMFLOAT3(+0.5f, -0.5f, +0.5f), DirectX::XMFLOAT4(+1.0f, +0.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.750f) },

        { DirectX::XMFLOAT3(-0.5f, +0.5f, -0.5f), DirectX::XMFLOAT4(-1.0f, +0.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.250f) }, // -X (left face)
        { DirectX::XMFLOAT3(-0.5f, +0.5f, +0.5f), DirectX::XMFLOAT4(-1.0f, +0.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.250f) },
        { DirectX::XMFLOAT3(-0.5f, -0.5f, +0.5f), DirectX::XMFLOAT4(-1.0f, +0.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.750f) },
        { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT4(-1.0f, +0.0f, +0.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.750f) },

        { DirectX::XMFLOAT3(-0.5f, +0.5f, +0.5f), DirectX::XMFLOAT4(+0.0f, +0.0f, +1.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.250f) }, // +Z (front face)
        { DirectX::XMFLOAT3(+0.5f, +0.5f, +0.5f), DirectX::XMFLOAT4(+0.0f, +0.0f, +1.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.250f) },
        { DirectX::XMFLOAT3(+0.5f, -0.5f, +0.5f), DirectX::XMFLOAT4(+0.0f, +0.0f, +1.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.750f) },
        { DirectX::XMFLOAT3(-0.5f, -0.5f, +0.5f), DirectX::XMFLOAT4(+0.0f, +0.0f, +1.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.750f) },

        { DirectX::XMFLOAT3(+0.5f, +0.5f, -0.5f), DirectX::XMFLOAT4(+0.0f, +0.0f, -1.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.250f) }, // -Z (back face)
        { DirectX::XMFLOAT3(-0.5f, +0.5f, -0.5f), DirectX::XMFLOAT4(+0.0f, +0.0f, -1.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.250f) },
        { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), DirectX::XMFLOAT4(+0.0f, +0.0f, -1.0f, 0.5f), DirectX::XMFLOAT2(0.250f, 0.750f) },
        { DirectX::XMFLOAT3(+0.5f, -0.5f, -0.5f), DirectX::XMFLOAT4(+0.0f, +0.0f, -1.0f, 0.5f), DirectX::XMFLOAT2(0.750f, 0.750f) },

    };
    const UINT16 cubeIndices[] = {
        // +Y (top face) - Original: 0,1,2 (CW), 0,2,3 (CW)
        0, 2, 1,    // Triangle 1: V0 (TL), V2 (BR), V1 (TR) - CCW
        0, 3, 2,    // Triangle 2: V0 (TL), V3 (BL), V2 (BR) - CCW

        // -Y (bottom face) - Original: 4,5,6 (CW), 4,6,7 (CW)
        4, 6, 5,    // Triangle 1: V4 (BL), V6 (TR), V5 (BR) - CCW
        4, 7, 6,    // Triangle 2: V4 (BL), V7 (TL), V6 (TR) - CCW

        // +Z (front face) - Original: 8,9,10 (CW), 8,10,11 (CW)
        8, 10, 9,   // Triangle 1: V8 (BL), V10 (TR), V9 (BR) - CCW
        8, 11, 10,  // Triangle 2: V8 (BL), V11 (TL), V10 (TR) - CCW

        // -Z (back face) - Original: 12,13,14 (CW), 12,14,15 (CW)
        12, 14, 13, // Triangle 1: V12 (BL), V14 (TR), V13 (BR) - CCW
        12, 15, 14, // Triangle 2: V12 (BL), V15 (TL), V14 (TR) - CCW

        // +X (right face) - Original: 16,17,18 (CW), 16,18,19 (CW)
        16, 18, 17, // Triangle 1: V16 (BF), V18 (TB), V17 (BB) - CCW
        16, 19, 18, // Triangle 2: V16 (BF), V19 (TF), V18 (TB) - CCW

        // -X (left face) - Original: 20,21,22 (CW), 20,22,23 (CW)
        20, 22, 21, // Triangle 1: V20 (BF), V22 (TB), V21 (BB) - CCW
        20, 23, 22  // Triangle 2: V20 (BF), V23 (TF), V22 (TB) - CCW
    };
    const UINT vertexBufferSize = sizeof(cubeVertices);
    const UINT indexBufferSize = sizeof(cubeIndices);
    const UINT numIndicesInArray = _countof(cubeIndices);
} // anonymous namespace


namespace Angaraka::Graphics::DirectX12 {

    BufferManager::BufferManager() {
        AGK_INFO("BufferManager: Constructor called.");
    }

    BufferManager::~BufferManager() {
        AGK_INFO("BufferManager: Destructor called.");
        if (m_mvpConstantBuffer && m_pCbvDataBegin) {
            m_mvpConstantBuffer->Unmap(0, nullptr);
            m_pCbvDataBegin = nullptr;
        }
    }

    bool BufferManager::Initialize(ID3D12Device* device) {
        AGK_INFO("BufferManager: Creating Vertex, Index, and Constant Buffers...");
        m_device = device;

        // --- Create Vertex Buffer ---
        CD3DX12_HEAP_PROPERTIES vertexHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        DXCall(m_device->CreateCommittedResource(
            &vertexHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &vertexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)));

        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
        DXCall(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, cubeVertices, vertexBufferSize);
        m_vertexBuffer->Unmap(0, nullptr); // Unmap after copying data

        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;
        AGK_INFO("BufferManager: Vertex Buffer Created and Data Uploaded. Stride: {0} bytes, Total Size: {1} bytes.",
            m_vertexBufferView.StrideInBytes, m_vertexBufferView.SizeInBytes);
        NAME_D3D12_OBJECT(m_vertexBuffer, "Vertex Buffer");


        // --- Create Index Buffer ---
        CD3DX12_HEAP_PROPERTIES indexHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
        DXCall(m_device->CreateCommittedResource(
            &indexHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &indexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_indexBuffer)));

        UINT8* pIndexDataBegin;
        DXCall(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
        memcpy(pIndexDataBegin, cubeIndices, indexBufferSize);
        m_indexBuffer->Unmap(0, nullptr); // Unmap after copying data

        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT; // Our indices are UINT16
        m_indexBufferView.SizeInBytes = indexBufferSize;
        m_numIndices = numIndicesInArray; // Store the count of indices

        AGK_INFO("BufferManager: Index Buffer Created and Data Uploaded. Format: R16_UINT, Total Size: {0} bytes, Num Indices: {1}.",
            m_indexBufferView.SizeInBytes, m_numIndices);
        NAME_D3D12_OBJECT(m_indexBuffer, "Index Buffer");


        // --- Create Constant Buffer ---
        // Constant buffers must be 256-byte aligned.
        const UINT constantBufferSize = (sizeof(ModelViewProjectionConstantBuffer) + 255) & ~255;

        CD3DX12_HEAP_PROPERTIES cbHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC cbResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

        DXCall(m_device->CreateCommittedResource(
            &cbHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &cbResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, // Upload heap resources must be in this state
            nullptr,
            IID_PPV_ARGS(&m_mvpConstantBuffer)));

        // Map the constant buffer. We don't unmap this until the app closes.
        DXCall(m_mvpConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
        AGK_INFO("BufferManager: Constant Buffer Created and Mapped.");
        NAME_D3D12_OBJECT(m_mvpConstantBuffer, "MVP Constant Buffer");

        return true;
    }

    void BufferManager::UpdateConstantBuffer(const ModelViewProjectionConstantBuffer& data) {
        if (m_pCbvDataBegin) {
            memcpy(m_pCbvDataBegin, &data, sizeof(ModelViewProjectionConstantBuffer));
        }
        else {
            AGK_ERROR("BufferManager: Attempted to update unmapped constant buffer.");
        }
    }

} // namespace Angaraka::Graphics::DirectX12