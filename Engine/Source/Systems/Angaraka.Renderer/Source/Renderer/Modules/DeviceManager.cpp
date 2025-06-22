// DeviceManager.cpp
// Implementation of Angaraka.Graphics.DirectX12.DeviceManager module.
module;
// Includes for internal implementation details
#include "Angaraka/GraphicsBase.hpp" // Assuming AGK_INFO, AGK_WARN, AGK_ERROR
#include <stdexcept>   // For std::runtime_error

// Necessary for DX12 debugging with ID3D12Debug
#ifdef _DEBUG
#include <dxgidebug.h>
#endif

module Angaraka.Graphics.DirectX12.DeviceManager; // Specifies this file belongs to the module

namespace Angaraka::Graphics::DirectX12 {

    namespace {
        String wstring_to_string(const std::wstring& wstr) {
            int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), NULL, 0, NULL, NULL);
            String str(count, 0);
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), &str[0], count, NULL, NULL);
            return str;
        }
    }

    DeviceManager::DeviceManager() {
        AGK_INFO("DeviceManager: Constructor called.");
    }

    DeviceManager::~DeviceManager() {
        AGK_INFO("DeviceManager: Destructor called.");
        // ComPtrs will automatically release resources.
    }

    // Private helper for string conversion


    bool DeviceManager::Initialize(bool debugEnabled) {
        AGK_INFO("DeviceManager: Creating DXGI Factory, D3D12 Device, and Command Queue...");

        // Enable Debug Layer
#ifdef _DEBUG
        if (debugEnabled) {
            Microsoft::WRL::ComPtr<ID3D12Debug3> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
                debugController->EnableDebugLayer();
                AGK_INFO("DirectX12 Debug Layer Enabled.");
            }
            else {
                AGK_WARN("DirectX12 Debug Layer could not be enabled.");
            }
        }
        else {
            AGK_DEBUG("DirectX12 Debug Layer not enabled. Requested by game...");
        }
#endif

        // Create DXGI Factory
        UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        DXCall(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory)));
        AGK_INFO("DeviceManager: DXGI Factory Created.");

        // Enumerate Adapters and Create Device
        Microsoft::WRL::ComPtr<IDXGIAdapter4> hardwareAdapter;
        for (UINT i = 0; m_dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&hardwareAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            hardwareAdapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // Don't use the Basic Render Driver adapter.
                continue;
            }

            // Check if adapter supports D3D12
            HRESULT hr = D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr);
            DXCall(hr)
            else {
                AGK_INFO("DeviceManager: Using adapter: {0}", wstring_to_string(desc.Description));
                break;
            }
            hardwareAdapter.Reset(); // If device creation failed, try next adapter
        }

        if (!hardwareAdapter) {
            AGK_ERROR("DeviceManager: No suitable hardware adapter found for D3D12.");
            return false;
        }

        DXCall(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
        NAME_D3D12_OBJECT(m_device, "Device"); // Set debug name

#ifdef _DEBUG
        {
            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, false); // Info can be noisy
            }
        }
#endif // _DEBUG

        // Create Command Queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        DXCall(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
        NAME_D3D12_OBJECT(m_commandQueue, "Command Queue");
        return true;
    }

} // namespace Angaraka::Graphics::DirectX12