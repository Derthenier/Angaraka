// ShaderManager.cpp
// Implementation of Angaraka.Graphics.DirectX12.ShaderManager module.
module;

// Includes for internal implementation details
#include "Angaraka/GraphicsBase.hpp"
#include <stdexcept>
#include <d3dcompiler.h> // For D3DCompileFromFile
#include <d3dcommon.h>

module Angaraka.Graphics.DirectX12.ShaderManager;

// Assume these are defined somewhere accessible (e.g., a common header or global)
// If not, you might need to pass them to Initialize or define them in this module.
// For now, assuming they are defined globally or via a shared header.
#define SHADER_PATH L"Shaders/SimpleShader.hlsl" // Example path, adjust as needed
#define VS_ENTRY_POINT "VSMain"
#define PS_ENTRY_POINT "PSMain"
#define VS_TARGET_VERSION "vs_5_0" // Or vs_6_0 for DirectX 12
#define PS_TARGET_VERSION "ps_5_0" // Or ps_6_0 for DirectX 12

namespace Angaraka::Graphics::DirectX12 {

    ShaderManager::ShaderManager() {
        AGK_INFO("ShaderManager: Constructor called.");
    }

    ShaderManager::~ShaderManager() {
        AGK_INFO("ShaderManager: Destructor called.");
    }

    bool ShaderManager::Initialize() {
        AGK_INFO("ShaderManager: Compiling Shaders from {0}...", "Shaders/SimpleShader.hlsl");

        UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DEBUG)
        // Enable better shader debugging with the debug layer.
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr;

        // Compile Vertex Shader
        hr = D3DCompileFromFile(
            SHADER_PATH,
            nullptr, // Defines
            D3D_COMPILE_STANDARD_FILE_INCLUDE, // Include handler for #include directives
            VS_ENTRY_POINT, // Entry point name
            VS_TARGET_VERSION, // Target shader model
            compileFlags,
            0, // Effect Flags
            &m_vertexShader,
            &errorBlob
        );
        if (FAILED(hr)) {
            if (errorBlob) {
                AGK_ERROR("ShaderManager: Vertex Shader Compilation Error: {0}", reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            DXCall(hr);
            return false;
        }
        AGK_INFO("ShaderManager: Vertex Shader Compiled.");

        // Compile Pixel Shader
        hr = D3DCompileFromFile(
            SHADER_PATH,
            nullptr, // Defines
            D3D_COMPILE_STANDARD_FILE_INCLUDE, // Include handler
            PS_ENTRY_POINT, // Entry point name
            PS_TARGET_VERSION, // Target shader model
            compileFlags,
            0, // Effect Flags
            &m_pixelShader,
            &errorBlob
        );
        if (FAILED(hr)) {
            if (errorBlob) {
                AGK_ERROR("ShaderManager: Pixel Shader Compilation Error: {0}", reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            DXCall(hr);
            return false;
        }
        AGK_INFO("ShaderManager: Pixel Shader Compiled.");

        return true;
    }

} // namespace Angaraka::Graphics::DirectX12