// PipelineManager.cpp
// Implementation of Angaraka.Graphics.DirectX12.PipelineManager module.
module;

// Includes for internal implementation details
#include "Angaraka/GraphicsBase.hpp"
#include <stdexcept>
#include <d3dcompiler.h> // For D3D12SerializeRootSignature

module Angaraka.Graphics.DirectX12.PipelineManager;

namespace Angaraka::Graphics::DirectX12 {
    namespace {
        const D3D12_STATIC_SAMPLER_DESC STATIC_POINT
        {
            D3D12_FILTER_MIN_MAG_MIP_POINT,                 // Filter
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // AddressU
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // AddressV
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // AddressW
            0.f,                                            // MipLODBias
            1,                                              // MaxAnisotropy
            D3D12_COMPARISON_FUNC_NONE,                     // ComparisonFunc
            D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,         // BorderColor
            0.f, D3D12_FLOAT32_MAX,                         // MinLOD, MaxLOD
            0, 0, D3D12_SHADER_VISIBILITY_PIXEL             // ShaderRegister, RegisterSpace, ShaderVisibility
        };

        const D3D12_STATIC_SAMPLER_DESC STATIC_LINEAR
        {
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,                // Filter
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // AddressU
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // AddressV
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // AddressW
            0.f,                                            // MipLODBias
            1,                                              // MaxAnisotropy
            D3D12_COMPARISON_FUNC_NONE,                     // ComparisonFunc
            D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,         // BorderColor
            0.f, D3D12_FLOAT32_MAX,                         // MinLOD, MaxLOD
            1, 0, D3D12_SHADER_VISIBILITY_PIXEL             // ShaderRegister, RegisterSpace, ShaderVisibility
        };

        const D3D12_STATIC_SAMPLER_DESC STATIC_ANISOTROPIC
        {
            D3D12_FILTER_ANISOTROPIC,                       // Filter
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // AddressU
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // AddressV
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // AddressW
            0.f,                                            // MipLODBias
            16,                                             // MaxAnisotropy
            D3D12_COMPARISON_FUNC_NONE,                     // ComparisonFunc
            D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,         // BorderColor
            0.f, D3D12_FLOAT32_MAX,                         // MinLOD, MaxLOD
            2, 0, D3D12_SHADER_VISIBILITY_PIXEL             // ShaderRegister, RegisterSpace, ShaderVisibility
        };
    }

    PipelineManager::PipelineManager() {
        AGK_INFO("PipelineManager: Constructor called.");
    }

    PipelineManager::~PipelineManager() {
        AGK_INFO("PipelineManager: Destructor called.");
    }

    bool PipelineManager::Initialize(
        ID3D12Device* device,
        ID3DBlob* vertexShader,
        ID3DBlob* pixelShader,
        const D3D12_INPUT_ELEMENT_DESC* inputLayout,
        UINT numInputElements
    ) {
        AGK_INFO("PipelineManager: Creating Root Signature and PSO...");
        m_device = device;

        // Create Root Signature
        CD3DX12_ROOT_PARAMETER rootParameters[2]{};

        // Parameter 0: Constant Buffer View (your MVP matrix)
        rootParameters[0].InitAsConstantBufferView(0); // b0 in HLSL

        // Parameter 1: Descriptor Table for Texture SRV
        CD3DX12_DESCRIPTOR_RANGE srvRange[1];
        srvRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 in HLSL
        rootParameters[1].InitAsDescriptorTable(1, srvRange);

        // Create Static Sampler (Recommended for texture filtering)
        const D3D12_STATIC_SAMPLER_DESC samplers[]
        {
            STATIC_POINT,
            STATIC_LINEAR,
            STATIC_ANISOTROPIC,
        };

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc {
            .NumParameters = _countof(rootParameters),                              // Number of root parameters
            .pParameters = rootParameters,                                          // Array of root parameters
            .NumStaticSamplers = _countof(samplers),                                // Number of static samplers
            .pStaticSamplers = &samplers[0],                                        // Array of static samplers
                                                                                    // Flags
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
        };

        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
        NAME_D3D12_OBJECT(m_rootSignature, "Root Signature");

        // Create Pipeline State Object (PSO)
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{
            .pRootSignature = m_rootSignature.Get(),
            .VS = CD3DX12_SHADER_BYTECODE(vertexShader),                        // Vertex Shader Bytecode
            .PS = CD3DX12_SHADER_BYTECODE(pixelShader),                         // Pixel Shader Bytecode
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),                    // Default blend state (no blending)
            .SampleMask = UINT_MAX,                                             // All bits set
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),          // Default rasterizer state
            .DepthStencilState {
                .DepthEnable = FALSE,                                           // No depth buffer for this simple example
                .StencilEnable = FALSE,                                         // No stencil buffer
            },
            .InputLayout = { inputLayout, numInputElements },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,    // Triangle list
            .NumRenderTargets = 1,                                              // One render target
            .SampleDesc {
                .Count = 1,                                                     // No MSAA
                .Quality = 0,                                                   // No MSAA
            },
            .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
        };
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Back buffer format

        DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
        NAME_D3D12_OBJECT(m_pipelineState, "Pipeline State Object");

        return true;
    }

} // namespace Angaraka::Graphics::DirectX12