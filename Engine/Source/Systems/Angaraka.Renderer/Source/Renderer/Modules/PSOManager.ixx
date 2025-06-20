// Angaraka.Graphics.DirectX12.PipelineManager.ixx
// Module for managing D3D12 Root Signature and Pipeline State Objects (PSOs).

module;

#include "Angaraka/GraphicsBase.hpp"
#include <d3dcommon.h>

export module Angaraka.Graphics.DirectX12.PipelineManager;

namespace Angaraka::Graphics::DirectX12 {

    export class PipelineManager {
    public:
        PipelineManager();
        ~PipelineManager();

        // Initializes the Root Signature and PSO.
        // Requires device, compiled vertex/pixel shaders, and input layout description.
        bool Initialize(
            ID3D12Device* device,
            ID3DBlob* vertexShader,
            ID3DBlob* pixelShader,
            const D3D12_INPUT_ELEMENT_DESC* inputLayout,
            UINT numInputElements
        );

        // Accessors for the managed D3D12 objects.
        ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

        ID3D12Device* m_device{ nullptr }; // Raw pointer to device owned by DeviceManager
    };

} // namespace Angaraka::Graphics::DirectX12