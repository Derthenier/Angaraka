// Angaraka.Graphics.DirectX12.CommandQueueAndListManager.ixx
// Module for managing D3D12 Command Allocators, Command Lists, and Fences.
module;

#include "Angaraka/GraphicsBase.hpp"

export module Angaraka.Graphics.DirectX12.CommandQueueAndListManager;

namespace Angaraka::Graphics::DirectX12 {

    export class CommandQueueAndListManager {
    public:
        CommandQueueAndListManager();
        ~CommandQueueAndListManager();

        // Initializes the Command Allocator, Command List, and Fence.
        bool Initialize(ID3D12Device* device);

        // Resets the command allocator and command list.
        // Returns the command list for recording commands.
        ID3D12GraphicsCommandList* Reset(ID3D12PipelineState* initialPSO = nullptr);

        // Closes the command list.
        void Close();

        // Executes the command list on the given command queue.
        void Execute(ID3D12CommandQueue* commandQueue);

        // Waits for the GPU to finish all commands up to the current fence value.
        void WaitForGPU(ID3D12CommandQueue* commandQueue);

        // Accessor for the main graphics command list.
        ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

        // Accessor for the fence (might be needed for external sync)
        ID3D12Fence* GetFence() const { return m_fence.Get(); }

        // Accessor for the current fence value
        UINT64 GetCurrentFenceValue() const { return m_fenceValue; }

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

        UINT64 m_fenceValue{ 0 };
        HANDLE m_fenceEvent{ nullptr }; // A Win32 event for CPU-GPU sync

        ID3D12Device* m_device{ nullptr }; // Raw pointer to device owned by DeviceManager
    };

} // namespace Angaraka::Graphics::DirectX12