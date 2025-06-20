module;

#include "Angaraka/GraphicsBase.hpp"

export module Angaraka.Graphics.DirectX12.DeviceManager;

namespace Angaraka::Graphics::DirectX12 {
    export class DeviceManager {
    public:
        DeviceManager();
        ~DeviceManager();

        // Initializes the DXGI Factory, D3D12 Device, and Command Queue.
        bool Initialize(bool debugEnabled = false);

        // Accessors for the created D3D12 objects
        ID3D12Device* GetDevice() const { return m_device.Get(); }
        ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
        IDXGIFactory7* GetDxgiFactory() const { return m_dxgiFactory.Get(); }

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory7> m_dxgiFactory;
        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    };

} // namespace Angaraka::Graphics::DirectX12