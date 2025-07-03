module;
// Angaraka.Graphics.DirectX12.BufferManager.ixx
// Module for managing D3D12 vertex, index, and constant buffers.

#include "Angaraka/GraphicsBase.hpp"

export module Angaraka.Graphics.DirectX12.BufferManager;

namespace Angaraka::Graphics::DirectX12 {

    export class BufferManager {
    public:
        BufferManager();
        ~BufferManager();

        // Initializes the buffers (vertex, index, constant).
        // Requires the D3D12 device.
        bool Initialize(ID3D12Device* device);

        // Accessors for buffer views.
        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_vertexBufferView; }
        const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_indexBufferView; }

        // Accessor for the GPU virtual address of the constant buffer.
        D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferGPUAddress() const {
            return m_mvpConstantBuffer->GetGPUVirtualAddress();
        }

        // Method to update the constant buffer data.
        void UpdateConstantBuffer(const MVPConstantBuffer& data);

        // Accessor for the number of indices to draw.
        unsigned int GetNumIndices() const { return m_numIndices; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        unsigned int m_numIndices{ 0 };

        Microsoft::WRL::ComPtr<ID3D12Resource> m_mvpConstantBuffer;
        UINT8* m_pCbvDataBegin = nullptr; // Pointer to the mapped constant buffer data

        ID3D12Device* m_device{ nullptr }; // Raw pointer to device owned by DeviceManager
    };

} // namespace Angaraka::Graphics::DirectX12