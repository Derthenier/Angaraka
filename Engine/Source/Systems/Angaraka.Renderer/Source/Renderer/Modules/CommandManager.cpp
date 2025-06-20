// CommandQueueAndListManager.cpp
// Implementation of Angaraka.Graphics.DirectX12.CommandQueueAndListManager module.
module;

// Includes for internal implementation details
#include "Angaraka/GraphicsBase.hpp"

module Angaraka.Graphics.DirectX12.CommandQueueAndListManager;

namespace Angaraka::Graphics::DirectX12 {

    CommandQueueAndListManager::CommandQueueAndListManager() {
        AGK_INFO("CommandQueueAndListManager: Constructor called.");
    }

    CommandQueueAndListManager::~CommandQueueAndListManager() {
        AGK_INFO("CommandQueueAndListManager: Destructor called.");
        if (m_fenceEvent != nullptr) {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }
    }

    bool CommandQueueAndListManager::Initialize(ID3D12Device* device) {
        AGK_INFO("CommandQueueAndListManager: Creating Command Allocator, Command List, and Fence...");

        m_device = device;

        // Create Command Allocator
        DXCall(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
        NAME_D3D12_OBJECT(m_commandAllocator, "Command Allocator");

        // Create Command List
        // Initially, the command list is associated with a null PSO.
        // It will be reset with the actual PSO in BeginFrame.
        DXCall(m_device->CreateCommandList(
            0, // NodeMask
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocator.Get(),
            nullptr, // Initial Pipeline State Object (PSO) - none yet
            IID_PPV_ARGS(&m_commandList)
        ));
        // Command lists are created in the "recording" state. Close it initially.
        DXCall(m_commandList->Close());
        NAME_D3D12_OBJECT(m_commandList, "Command List");

        // Create Fence
        DXCall(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        AGK_INFO("CommandQueueAndListManager: D3D12 Fence Created.");
        m_fenceValue = 0; // Initialize fence value

        // Create a Win32 event to be fired when the fence reaches a certain value.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr) {
            HRESULT_FROM_WIN32(GetLastError()); // Get the last error code from Win32
            AGK_ERROR("CommandQueueAndListManager: Failed to create fence event handle (HRESULT: {0:x}).", (int)HRESULT_FROM_WIN32(GetLastError()));
            return false;
        }
        AGK_INFO("CommandQueueAndListManager: D3D12 Fence Event Created.");

        return true;
    }

    ID3D12GraphicsCommandList* CommandQueueAndListManager::Reset(ID3D12PipelineState* initialPSO) {
        DXCall(m_commandAllocator->Reset());
        DXCall(m_commandList->Reset(m_commandAllocator.Get(), initialPSO));
        return m_commandList.Get();
    }

    void CommandQueueAndListManager::Close() {
        DXCall(m_commandList->Close());
    }

    void CommandQueueAndListManager::Execute(ID3D12CommandQueue* commandQueue) {
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    }

    void CommandQueueAndListManager::WaitForGPU(ID3D12CommandQueue* commandQueue) {
        // Increment the fence value to signal the GPU to a new point.
        const UINT64 currentFenceValue = m_fenceValue;
        DXCall(commandQueue->Signal(m_fence.Get(), currentFenceValue));
        m_fenceValue++; // Increment for the next fence point

        // Wait until the GPU has completed commands up to this fence point.
        if (m_fence->GetCompletedValue() < currentFenceValue) {
            DXCall(m_fence->SetEventOnCompletion(currentFenceValue, m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

} // namespace Angaraka::Graphics::DirectX12