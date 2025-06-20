// SwapChainManager.cpp
// Implementation of Angaraka.Graphics.DirectX12.SwapChainManager module.
module;

// Includes for internal implementation details
#include "Angaraka/GraphicsBase.hpp"
#include <stdexcept>

module Angaraka.Graphics.DirectX12.SwapChainManager;

namespace Angaraka::Graphics::DirectX12 {

    SwapChainManager::SwapChainManager() {
        AGK_INFO("SwapChainManager: Constructor called.");
    }

    SwapChainManager::~SwapChainManager() {
        AGK_INFO("SwapChainManager: Destructor called.");
        // ComPtrs automatically release resources.
    }

    bool SwapChainManager::Initialize(
        ID3D12Device* device,
        ID3D12CommandQueue* commandQueue,
        IDXGIFactory7* dxgiFactory,
        HWND windowHandle,
        unsigned int width,
        unsigned int height
    ) {
        AGK_INFO("SwapChainManager: Initializing Swap Chain ({0}x{1})...", width, height);

        m_device = device;
        m_commandQueue = commandQueue;
        m_dxgiFactory = dxgiFactory;
        m_windowHandle = windowHandle;
        m_width = width;
        m_height = height;

        // Describe and create the RTV descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameBufferCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // RTV heaps don't need to be shader visible

        DXCall(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));
        NAME_D3D12_OBJECT(m_rtvDescriptorHeap, "RTV Descriptor Heap");

        // Cache the size of the RTV descriptor (needed for stepping through the heap)
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        AGK_INFO("SwapChainManager: RTV Descriptor Size Cached: {0}", m_rtvDescriptorSize);

        // Create Swap Chain
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = m_width;
        swapChainDesc.Height = m_height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = FrameBufferCount;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
        DXCall(m_dxgiFactory->CreateSwapChainForHwnd(
            m_commandQueue,
            m_windowHandle,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
        ));
        DXCall(swapChain1.As(&m_swapChain));
        AGK_INFO("SwapChainManager: DXGI Swap Chain Created.");
        DXCall(m_dxgiFactory->MakeWindowAssociation(m_windowHandle, DXGI_MWA_NO_ALT_ENTER));

        // Create initial render targets
        CreateRenderTargets();

        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
        return true;
    }

    void SwapChainManager::CreateRenderTargets() {
        AGK_INFO("SwapChainManager: Creating Render Targets...");
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        for (UINT i = 0; i < FrameBufferCount; ++i) {
            // Get the address of the i-th back buffer in the swap chain
            DXCall(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
            NAME_D3D12_OBJECT_INDEXED(m_renderTargets[i], i, "Render Target Resource");
            // Create a render target view for the current back buffer
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
            // Offset the RTV handle to the next descriptor slot
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

    void SwapChainManager::Resize(unsigned int newWidth, unsigned int newHeight) {
        AGK_INFO("SwapChainManager: Resizing to {0}x{1}.", newWidth, newHeight);

        // Release old render target views and resources
        for (UINT i = 0; i < FrameBufferCount; ++i) {
            m_renderTargets[i].Reset();
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        DXCall(m_swapChain->GetDesc(&swapChainDesc)); // Get current swap chain description

        // Resize the swap chain buffers
        DXCall(m_swapChain->ResizeBuffers(
            FrameBufferCount,
            newWidth,
            newHeight,
            swapChainDesc.BufferDesc.Format, // Keep the same format
            swapChainDesc.Flags
        ));

        m_width = newWidth;
        m_height = newHeight;
        m_currentBackBufferIndex = 0; // After resize, the current back buffer index is usually 0

        // Recreate RTVs for the new swap chain buffers
        CreateRenderTargets();

        AGK_INFO("SwapChainManager: Swap chain and render targets resized.");
    }

    void SwapChainManager::Present() {
        DXCall(m_swapChain->Present(0, 0));
        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex(); // Update current back buffer index
    }

    D3D12_CPU_DESCRIPTOR_HANDLE SwapChainManager::GetCurrentRTVHandle() const {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            m_currentBackBufferIndex,
            m_rtvDescriptorSize
        );
        return rtvHandle;
    }

} // namespace Angaraka::Graphics::DirectX12