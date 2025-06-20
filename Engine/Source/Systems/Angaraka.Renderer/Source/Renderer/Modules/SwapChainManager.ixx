// Angaraka.Graphics.DirectX12.SwapChainManager.ixx
// Module for managing the DXGI Swap Chain and Render Targets.
module;

// Standard library includes and DirectX headers needed for the interface
#include "Angaraka/GraphicsBase.hpp"
#include <windows.h> // For HWND
#include <wrl/client.h> // For Microsoft::WRL::ComPtr
#include <d3d12.h>    // For D3D12_CPU_DESCRIPTOR_HANDLE, ID3D12Resource etc.
#include <dxgi1_6.h>  // For IDXGISwapChain3

export module Angaraka.Graphics.DirectX12.SwapChainManager;

namespace Angaraka::Graphics::DirectX12 {

    export class SwapChainManager {
    public:
        SwapChainManager();
        ~SwapChainManager();

        // Initializes the Swap Chain and creates render targets.
        // Needs pointers to device, command queue, and factory created by DeviceManager.
        bool Initialize(
            ID3D12Device* device,
            ID3D12CommandQueue* commandQueue,
            IDXGIFactory7* dxgiFactory,
            HWND windowHandle,
            unsigned int width,
            unsigned int height
        );

        // Handles resizing the swap chain and recreating render targets.
        void Resize(unsigned int newWidth, unsigned int newHeight);

        // Presents the back buffer to the screen.
        void Present();

        // Accessors for the current back buffer's RTV CPU handle
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const;

        // Accessor for the current back buffer index
        unsigned int GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }

        // Accessor for the RTV descriptor size (needed for offset calculations)
        unsigned int GetRTVDescriptorSize() const { return m_rtvDescriptorSize; }

        // Accessor to the device (needed for creating render targets during resize)
        ID3D12Device* GetDevice() const { return m_device; }


        static const unsigned int FrameBufferCount = 2; // For double buffering

        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameBufferCount];
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;

        // Raw pointers to objects owned by DeviceManager, used for creation/interaction
        // These are not ComPtrs because we don't own their lifetime here.
        ID3D12Device* m_device{ nullptr };
        ID3D12CommandQueue* m_commandQueue{ nullptr };
        IDXGIFactory7* m_dxgiFactory{ nullptr };

        HWND m_windowHandle{ nullptr };
        unsigned int m_width{ 0 };
        unsigned int m_height{ 0 };
        unsigned int m_rtvDescriptorSize{ 0 };
        unsigned int m_currentBackBufferIndex{ 0 };

        // Private helper to create/recreate render target views
        void CreateRenderTargets();
    };

} // namespace Angaraka::Graphics::DirectX12