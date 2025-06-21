// Angaraka.Graphics.DirectX12.ixx
// (Main Module Interface for DirectX12GraphicsSystem)
module;

#include "Angaraka/GraphicsBase.hpp" // For AGK_INFO, AGK_ERROR, etc.
#include <windows.h>
#include <string>
#include <memory>    // For std::unique_ptr
#include <DirectXMath.h>
#include <wrl/client.h>

export module Angaraka.Graphics.DirectX12;

import Angaraka.Core.GraphicsFactory;

import Angaraka.Graphics.DirectX12.DeviceManager;
import Angaraka.Graphics.DirectX12.SwapChainManager;
import Angaraka.Graphics.DirectX12.CommandQueueAndListManager;
import Angaraka.Graphics.DirectX12.ShaderManager;
import Angaraka.Graphics.DirectX12.PipelineManager;
import Angaraka.Graphics.DirectX12.BufferManager;

import Angaraka.Graphics.DirectX12.Texture;
import Angaraka.Camera;

namespace Angaraka { // Use the Angaraka namespace here

    export class DirectX12GraphicsSystem
    {
    public:
        DirectX12GraphicsSystem();
        ~DirectX12GraphicsSystem();

#ifdef _WIN64
        bool Initialize(HWND windowHandle, unsigned int width, unsigned int height, bool debugEnabled = false);
#else
        bool Initialize(void* windowHandle, unsigned int width, unsigned int height, bool debugEnabled = false);
#endif
        void Shutdown();
        void BeginFrame(float deltaTime, float r, float g, float b, float a);
        void BeginFrame(float deltaTime, Graphics::DirectX12::TextureResource* texture, float r, float g, float b, float a);
        void EndFrame();
        void OnWindowResize(unsigned int newWidth, unsigned int newHeight);

        inline Angaraka::Camera* GetCamera() { return m_camera.get(); }
        inline Graphics::DirectX12::TextureManager* GetTextureManager() const { return m_textureManager.get(); }

        std::shared_ptr<Core::GraphicsResourceFactory> GetGraphicsFactory();

    private:
        void BeginFrameStartInternal(ID3D12GraphicsCommandList* commandList, float deltaTime);
        void BeginFrameEndInternal(ID3D12GraphicsCommandList* commandList, float r, float g, float b, float a);

    private:
        std::unique_ptr<Graphics::DirectX12::DeviceManager> m_deviceManager;
        std::unique_ptr<Graphics::DirectX12::SwapChainManager> m_swapChainManager;
        std::unique_ptr<Graphics::DirectX12::CommandQueueAndListManager> m_commandManager;
        std::unique_ptr<Graphics::DirectX12::ShaderManager> m_shaderManager;
        std::unique_ptr<Graphics::DirectX12::PipelineManager> m_pipelineManager;
        std::unique_ptr<Graphics::DirectX12::BufferManager> m_bufferManager;

        std::shared_ptr<Graphics::DirectX12::TextureManager> m_textureManager;
        std::shared_ptr<Angaraka::Camera> m_camera;

        HWND m_windowHandle{ nullptr };
        unsigned int m_width{ 0 };
        unsigned int m_height{ 0 };
        float m_elapsedTime = 0.0f;
    };


    export class DirectX12ResourceFactory : public Core::IGraphicsResourceFactory<DirectX12ResourceFactory> {
    public:
        inline DirectX12ResourceFactory(DirectX12GraphicsSystem* graphicsSystem)
            : m_graphicsSystem(graphicsSystem) {
        }

        inline std::shared_ptr<Core::Resource> CreateTextureImpl(const Core::AssetDefinition& asset, void* context) {
            std::shared_ptr<Graphics::DirectX12::TextureResource> textureResource = std::make_shared<Graphics::DirectX12::TextureResource>(asset.id);
            textureResource->Load(asset.path, context);
            return textureResource;
        }

        inline std::shared_ptr<Core::Resource> CreateMeshImpl(const Core::AssetDefinition& asset, void* context) {
            // TODO: Implement mesh creation
            return nullptr;
        }

        inline std::shared_ptr<Core::Resource> CreateMaterialImpl(const Core::AssetDefinition& asset, void* context) {
            // TODO: Implement material creation  
            return nullptr;
        }

        inline std::shared_ptr<Core::Resource> CreateSoundImpl(const Core::AssetDefinition& asset, void* context) {
            // TODO: Implement sound creation
            return nullptr;
        }

    private:
        DirectX12GraphicsSystem* m_graphicsSystem;
    };

} // namespace Angaraka