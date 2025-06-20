// Angaraka.Graphics.DirectX12.ShaderManager.ixx
// Module for managing shader compilation.
module;

#include "Angaraka/GraphicsBase.hpp"
#include <d3dcommon.h>

export module Angaraka.Graphics.DirectX12.ShaderManager;

namespace Angaraka::Graphics::DirectX12 {

    export class ShaderManager {
    public:
        ShaderManager();
        ~ShaderManager();

        // Initializes the shader manager by compiling the necessary shaders.
        bool Initialize();

        // Accessors for the compiled shader bytecode.
        ID3DBlob* GetVertexShader() const { return m_vertexShader.Get(); }
        ID3DBlob* GetPixelShader() const { return m_pixelShader.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShader;
    };

} // namespace Angaraka::Graphics::DirectX12