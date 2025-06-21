// DirectX12GraphicsSystem.cpp
// Implementation of Angaraka.Graphics.DirectX12 module.
module;

#define TEST_IMAGE "Assets/uv-grid-texture.png" // Path to a test image for loading


// Includes for internal implementation details
#include "Angaraka/GraphicsBase.hpp"
#include <stdexcept>

module Angaraka.Graphics.DirectX12; // Specifies this file belongs to the module

import Angaraka.Core.Resources;

// Global/Static Data (specific to this implementation unit)
// This will remain here for now, as it's geometry data.
namespace {
    Angaraka::Graphics::DirectX12::ModelViewProjectionConstantBuffer mvpCPUData{};
    Angaraka::Graphics::DirectX12::TextureResource* dummyTextureResource = nullptr;

    // Define the input layout for our vertex structure
    const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        // If you add color to your Vertex struct:
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    const UINT numInputElements = _countof(inputLayout);

} // anonymous namespace


namespace Angaraka { // Use the Angaraka namespace here

    DirectX12GraphicsSystem::DirectX12GraphicsSystem() {
        AGK_INFO("DirectX12GraphicsSystem: Constructor called.");
        // Initialize the unique_ptr for the DeviceManager
        m_deviceManager = std::make_unique<Graphics::DirectX12::DeviceManager>();
        m_swapChainManager = std::make_unique<Graphics::DirectX12::SwapChainManager>();
        m_commandManager = std::make_unique<Graphics::DirectX12::CommandQueueAndListManager>();
        m_shaderManager = std::make_unique<Graphics::DirectX12::ShaderManager>();
        m_pipelineManager = std::make_unique<Graphics::DirectX12::PipelineManager>();
        m_bufferManager = std::make_unique<Graphics::DirectX12::BufferManager>();

        m_textureManager = std::make_shared<Graphics::DirectX12::TextureManager>();
        m_camera = std::make_shared<Angaraka::Camera>();
    }

    DirectX12GraphicsSystem::~DirectX12GraphicsSystem() {
        AGK_INFO("DirectX12GraphicsSystem: Destructor called.");
        // Shutdown handles unsubscription and other cleanup
        Shutdown();
    }

    // --- Main Initialize method of DirectX12GraphicsSystem ---
#ifdef _WIN32
    bool DirectX12GraphicsSystem::Initialize(HWND windowHandle, unsigned int width, unsigned int height, bool debugEnabled) {
        m_windowHandle = windowHandle;
        m_width = width;
        m_height = height;

        AGK_INFO("DirectX12GraphicsSystem: Initializing for window {0} ({1}x{2})...",
            (void*)m_windowHandle, m_width, m_height);


        if (!m_deviceManager->Initialize(debugEnabled)) {
            return false;
        }
        AGK_INFO("DirectX12GraphicsSystem: DeviceManager initialized.");

        if (!m_swapChainManager->Initialize(
            m_deviceManager->GetDevice(),
            m_deviceManager->GetCommandQueue(),
            m_deviceManager->GetDxgiFactory(),
            m_windowHandle,
            m_width,
            m_height
        )) {
            return false;
        }
        AGK_INFO("DirectX12GraphicsSystem: SwapChainManager initialized.");

        if (!m_commandManager->Initialize(m_deviceManager->GetDevice())) {
            return false;
        }
        AGK_INFO("DirectX12GraphicsSystem: CommandQueueAndListManager initialized.");

        if (!m_shaderManager->Initialize()) {
            return false;
        }
        AGK_INFO("DirectX12GraphicsSystem: ShaderManager initialized.");

        if (!m_pipelineManager->Initialize(
            m_deviceManager->GetDevice(),
            m_shaderManager->GetVertexShader(),
            m_shaderManager->GetPixelShader(),
            inputLayout,      // Pass the static inputLayout data
            numInputElements  // Pass the count of input elements
        )) {
            return false;
        }
        AGK_INFO("DirectX12GraphicsSystem: PipelineManager initialized.");

        // --- NEW: Initialize the BufferManager ---
        if (!m_bufferManager->Initialize(m_deviceManager->GetDevice())) {
            return false;
        }
        AGK_INFO("DirectX12GraphicsSystem: BufferManager initialized.");


        // Initialize the TextureManager
        ID3D12GraphicsCommandList* commandList = m_commandManager->Reset(nullptr);
        if (!m_textureManager->Initialize(m_deviceManager->GetDevice(), commandList)) // Pass your D3D12 device and command list
        {
            AGK_ERROR("Failed to initialize Texture Manager.");
            return false;
        }


        // Example: Load a dummy texture (replace with your actual texture path)
        // You might need to place a PNG or DDS file in your RPGGame's Data/Assets folder
        // or directly in the Angaraka/Build/Debug folder for testing.
        std::string dummyTexturePath = TEST_IMAGE; // Adjust path as needed
        dummyTextureResource = new Angaraka::Graphics::DirectX12::TextureResource(dummyTexturePath);
        if (dummyTextureResource->Load(dummyTexturePath))
        {
            AGK_INFO("Dummy texture loaded and uploaded to GPU!");
        }
        else
        {
            AGK_WARN("Could not load dummy texture from path: {}", dummyTexturePath);
        }

        m_commandManager->Close();
        m_commandManager->Execute(m_deviceManager->GetCommandQueue());
        m_commandManager->WaitForGPU(m_deviceManager->GetCommandQueue());

        // --- IMPORTANT: Now that GPU is finished, safely release the temporary upload heaps ---
        m_textureManager->ClearUploadHeaps();

        // Aspect ratio calculated based on window size
        m_camera->Initialize(
            DirectX::XM_PIDIV4, // 45 degrees FOV
            static_cast<float>(m_width) / static_cast<float>(m_height), // Aspect ratio
            0.1f, // Near plane
            100.0f // Far plane
        );
        AGK_INFO("DirectX12GraphicsSystem: Camera initialized.");

        mvpCPUData.model = DirectX::XMMatrixIdentity();
        mvpCPUData.view = DirectX::XMMatrixIdentity();
        mvpCPUData.projection = DirectX::XMMatrixIdentity();

        AGK_INFO("DirectX12GraphicsSystem: Initialization complete.");
        return true;
    }
#else
    bool DirectX12GraphicsSystem::Initialize(void* windowHandle, unsigned int width, unsigned int height, bool debugEnabled) {
        AGK_ERROR("DirectX12GraphicsSystem: Not implemented for non-Windows platforms.");
        return false;
    }
#endif

    void DirectX12GraphicsSystem::Shutdown() {
        // Ensures all pending GPU commands are completed before releasing resources.
        // This is vital for a clean shutdown.
        if (m_deviceManager && m_commandManager) {
            m_commandManager->WaitForGPU(m_deviceManager->GetCommandQueue());
        }

        AGK_INFO("DirectX12GraphicsSystem: Shutting down.");

        // The unique_ptrs for our managers (m_commandManager, m_swapChainManager, m_deviceManager)
        // will automatically call the destructors of the managed objects when DirectX12GraphicsSystem
        // is destroyed or when .reset() is called. Explicitly calling .reset() here ensures
        // they are released in a defined order (if that specific order is critical, e.g., releasing
        // objects that depend on others).

        for (long i{ 0 }; i < m_textureManager.use_count(); ++i) {
            m_textureManager.reset();
        }

        m_bufferManager.reset();
        m_shaderManager.reset();
        m_commandManager.reset();
        m_swapChainManager.reset();
        m_deviceManager.reset();

        for (long i{ 0 }; i < m_camera.use_count(); ++i) {
            m_camera.reset();
        }
    }

    void DirectX12GraphicsSystem::BeginFrame(float deltaTime, float r, float g, float b, float a) {        // --- Update InputManager state and broadcast events ---
        // Mouse movement is now event-driven and handled in the subscription callback.
        // We only ensure camera updates its matrices after all input processing
        m_camera->Update(deltaTime);

        unsigned int currentBackBufferIndex = m_swapChainManager->GetCurrentBackBufferIndex();

        m_elapsedTime += deltaTime;

        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(m_elapsedTime * 0.5f); // Rotate around Y-axis
        DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(0.0f, 0.0f, 5.0f); // Move away from camera
        mvpCPUData.model = DirectX::XMMatrixTranspose(scale * rotation * translation); // Combine transformations

        mvpCPUData.view = DirectX::XMMatrixTranspose(m_camera->GetViewMatrix());
        mvpCPUData.projection = DirectX::XMMatrixTranspose(m_camera->GetProjectionMatrix());

        // Wait for previous frame to finish before resetting command list
        m_commandManager->WaitForGPU(m_deviceManager->GetCommandQueue());

        // Reset command allocator and command list
        ID3D12GraphicsCommandList* commandList = m_commandManager->Reset(m_pipelineManager->GetPipelineState());
        commandList->SetGraphicsRootSignature(m_pipelineManager->GetRootSignature());

        // Set the descriptor heaps (important for SRVs)
        // You must set the SRV heap before trying to set descriptor tables that use it.
        ID3D12DescriptorHeap* ppHeaps[] = { m_textureManager->GetSrvHeap() }; // Assuming TextureManager has a getter for its SRV heap
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        m_bufferManager->UpdateConstantBuffer(mvpCPUData);

        commandList->SetGraphicsRootConstantBufferView(0, m_bufferManager->GetConstantBufferGPUAddress());

        if (dummyTextureResource && dummyTextureResource->GetSrvIndex() >= 0)
        {
            // Bind the texture SRV (Root Parameter 1)
            // You stored the texture's SRV descriptor CPU handle in m_DummyTexture->SrvDescriptor
            // Now you need its GPU handle.
            CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle(m_textureManager->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart(), dummyTextureResource->GetSrvIndex(), m_textureManager->GetSrvDescriptorSize());

            UINT srvIndex = dummyTextureResource->GetSrvIndex(); // Assuming you store the index where it was allocated
            CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrvHandle(m_textureManager->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
            gpuSrvHandle.Offset(srvIndex, m_textureManager->GetSrvDescriptorSize());


            commandList->SetGraphicsRootDescriptorTable(1, gpuSrvHandle); // Root Parameter 1 binds the descriptor table
        }

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &m_bufferManager->GetVertexBufferView());
        commandList->IASetIndexBuffer(&m_bufferManager->GetIndexBufferView());

        D3D12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
        D3D12_RECT scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height));
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        D3D12_RESOURCE_BARRIER renderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_swapChainManager->m_renderTargets[currentBackBufferIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        commandList->ResourceBarrier(1, &renderTargetBarrier);

        // Get the RTV handle from SwapChainManager
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_swapChainManager->GetCurrentRTVHandle();
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        const float clearColor[] = { r,g,b,a };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        commandList->DrawIndexedInstanced(
            m_bufferManager->GetNumIndices(),
            1,
            0,
            0,
            0
        );
    }

    void DirectX12GraphicsSystem::EndFrame() {
        D3D12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_swapChainManager->m_renderTargets[m_swapChainManager->GetCurrentBackBufferIndex()].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );
        m_commandManager->GetCommandList()->ResourceBarrier(1, &presentBarrier);

        m_commandManager->Close();
        m_commandManager->Execute(m_deviceManager->GetCommandQueue());

        m_swapChainManager->Present();
    }

    void DirectX12GraphicsSystem::OnWindowResize(unsigned int newWidth, unsigned int newHeight) {
        AGK_INFO("DirectX12GraphicsSystem: Window resized to {0}x{1}.", newWidth, newHeight);

        // Ensure all commands for the previous frame are finished before resizing
        m_commandManager->WaitForGPU(m_deviceManager->GetCommandQueue());

        // Resize the swap chain
        m_swapChainManager->Resize(newWidth, newHeight);

        m_width = newWidth; // Update main system's dimensions
        m_height = newHeight;

        if (m_camera) {
            m_camera->SetLens(
                DirectX::XM_PIDIV4,
                static_cast<float>(m_width) / static_cast<float>(m_height),
                0.1f,
                100.0f
            );
            // No need to call m_camera->Update() here unless it specifically re-calculates projection
            // which SetLens already does.
        }
    }
} // namespace Angaraka
