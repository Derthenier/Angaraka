// Angaraka.Graphics.DirectX12.ixx
// (Main Module Interface for DirectX12GraphicsSystem)
module;

#include "Angaraka/GraphicsBase.hpp" // For AGK_INFO, AGK_ERROR, etc.
#include <windows.h>
#include <string>
#include <memory>    // For std::unique_ptr
#include <DirectXMath.h>
#include <SimpleMath.h>
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
import Angaraka.Graphics.DirectX12.Mesh;
import Angaraka.Camera;
import Angaraka.Core.Config;

namespace Angaraka { // Use the Angaraka namespace here

    export class DirectX12GraphicsSystem
    {
    public:
        DirectX12GraphicsSystem();
        ~DirectX12GraphicsSystem();

#ifdef _WIN64
        bool Initialize(HWND windowHandle, const Config::EngineConfig& config);
#else
        bool Initialize(void* windowHandle, const Config::EngineConfig& config);
#endif
        void Shutdown();
        void BeginFrame(F32 deltaTime);
        void EndFrame();
        void Present();

        void RenderTexture(Graphics::DirectX12::TextureResource* texture);
        void RenderMesh(Graphics::DirectX12::MeshResource* mesh);

        void OnWindowResize(unsigned int newWidth, unsigned int newHeight);

        inline Angaraka::Camera* GetCamera() { return m_camera.get(); }
        inline Graphics::DirectX12::TextureManager* GetTextureManager() const { return m_textureManager.get(); }
        inline Graphics::DirectX12::MeshManager* GetMeshManager() const { return m_meshManager.get(); }

        Reference<Core::GraphicsResourceFactory> GetGraphicsFactory();

        inline void DisplayStats() const {
            m_meshManager->LogMemoryUsage();
        }

    private:
        Scope<Graphics::DirectX12::DeviceManager> m_deviceManager;
        Scope<Graphics::DirectX12::SwapChainManager> m_swapChainManager;
        Scope<Graphics::DirectX12::CommandQueueAndListManager> m_commandManager;
        Scope<Graphics::DirectX12::ShaderManager> m_shaderManager;
        Scope<Graphics::DirectX12::PipelineManager> m_pipelineManager;
        Scope<Graphics::DirectX12::BufferManager> m_bufferManager;

        Reference<Graphics::DirectX12::TextureManager> m_textureManager;
        Reference<Graphics::DirectX12::MeshManager> m_meshManager;
        Reference<Angaraka::Camera> m_camera;

        DirectX::SimpleMath::Color m_clearColor{};

        HWND m_windowHandle{ nullptr };
        unsigned int m_width{ 0 };
        unsigned int m_height{ 0 };
        F32 m_elapsedTime = 0.0f;
    };


    export class DirectX12ResourceFactory : public Core::IGraphicsResourceFactory<DirectX12ResourceFactory> {
    public:
        inline DirectX12ResourceFactory(DirectX12GraphicsSystem* graphicsSystem)
            : m_graphicsSystem(graphicsSystem) {
        }

        inline Reference<Core::Resource> CreateTextureImpl(const Core::AssetDefinition& asset, void* context) {
            Reference<Graphics::DirectX12::TextureResource> textureResource = CreateReference<Graphics::DirectX12::TextureResource>(asset.id);
            textureResource->Load(asset.path, context);
            return textureResource;
        }

        inline Reference<Core::Resource> CreateMeshImpl(const Core::AssetDefinition& asset, void* context) {
            Reference<Graphics::DirectX12::MeshResource> meshResource = CreateReference<Graphics::DirectX12::MeshResource>(asset.id);
            meshResource->Load(asset.path, context);
            return meshResource;
        }

        inline Reference<Core::Resource> CreateMaterialImpl(const Core::AssetDefinition& asset, void* context) {
            // TODO: Implement material creation  
            return nullptr;
        }

        inline Reference<Core::Resource> CreateSoundImpl(const Core::AssetDefinition& asset, void* context) {
            // TODO: Implement sound creation
            return nullptr;
        }

    private:
        DirectX12GraphicsSystem* m_graphicsSystem;
    };

} // namespace Angaraka