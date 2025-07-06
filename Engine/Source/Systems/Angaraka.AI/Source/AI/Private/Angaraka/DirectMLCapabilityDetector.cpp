// DirectMLCapabilityDetector.cpp
// Implementation of DirectML capability detection
#include <Angaraka/Base.hpp>
#include "Angaraka/DirectMLCapabilityDetector.hpp"
#include <d3d12.h> // Core DX12 header
#include <dxgi1_6.h> // DXGI for adapter enumeration and swap chain
#include <wrl/client.h>     // For Microsoft::WRL::ComPtr



using Microsoft::WRL::ComPtr;

namespace Angaraka::AI {

    DirectMLCapabilities DirectMLCapabilityDetector::DetectCapabilities(ID3D12Device* device) {
        DirectMLCapabilities caps;

        if (!device) {
            caps.detectionError = "No D3D12 device provided";
            AGK_ERROR("DirectML capability detection: No D3D12 device provided");
            return caps;
        }

        // Get adapter information first
        GetAdapterInfo(device, caps);

        // Test DirectML device creation
        caps.isDirectMLSupported = TestDirectMLDevice(device);

        // Test MetaCommand support
        caps.areMetaCommandsSupported = TestMetaCommandSupport(device);

        // Determine recommendations
        if (caps.isDirectMLSupported && caps.areMetaCommandsSupported) {
            caps.isGPUAccelerationRecommended = true;
            caps.recommendedExecutionProvider = "DirectML";
        }
        else if (caps.isDirectMLSupported) {
            caps.isGPUAccelerationRecommended = false;
            caps.recommendedExecutionProvider = "CPU";
            caps.detectionError = "DirectML supported but MetaCommands not available";
        }
        else {
            caps.isGPUAccelerationRecommended = false;
            caps.recommendedExecutionProvider = "CPU";
            caps.detectionError = "DirectML not supported on this system";
        }

        return caps;
    }

    bool DirectMLCapabilityDetector::TestDirectMLDevice(ID3D12Device* device) {
        try {
            ComPtr<IDMLDevice> dmlDevice;

            // Create DirectML device
            HRESULT hr = DMLCreateDevice(
                device,
                DML_CREATE_DEVICE_FLAG_NONE,
                IID_PPV_ARGS(&dmlDevice)
            );

            if (SUCCEEDED(hr) && dmlDevice) {
                AGK_INFO("DirectML device creation successful");
                return true;
            }
            else {
                AGK_WARN("DirectML device creation failed: HRESULT = 0x{:X}", hr);
                return false;
            }
        }
        catch (const std::exception& e) {
            AGK_ERROR("Exception during DirectML device test: {}", e.what());
            return false;
        }
    }

    bool DirectMLCapabilityDetector::TestMetaCommandSupport(ID3D12Device* device) {
        try {
            // Check if the device supports MetaCommands at all
            ComPtr<ID3D12Device5> device5;
            HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&device5));

            if (FAILED(hr)) {
                AGK_WARN("Device doesn't support ID3D12Device5 interface (required for MetaCommands)");
                return false;
            }

            // Try to enumerate MetaCommands
            UINT numMetaCommands = 0;
            hr = device5->EnumerateMetaCommands(&numMetaCommands, nullptr);

            if (SUCCEEDED(hr) && numMetaCommands > 0) {
                AGK_INFO("System supports {} MetaCommands", numMetaCommands);

                // Get the list of supported MetaCommands
                std::vector<D3D12_META_COMMAND_DESC> metaCommandDescs(numMetaCommands);
                hr = device5->EnumerateMetaCommands(&numMetaCommands, metaCommandDescs.data());

                if (SUCCEEDED(hr)) {
                    AGK_INFO("Successfully enumerated MetaCommands");
                    return true;
                }
                else {
                    AGK_WARN("Failed to enumerate MetaCommand details: HRESULT = 0x{:X}", hr);
                    return false;
                }
            }
            else {
                AGK_WARN("No MetaCommands supported on this system");
                return false;
            }
        }
        catch (const std::exception& e) {
            AGK_ERROR("Exception during MetaCommand test: {}", e.what());
            return false;
        }
    }

    void DirectMLCapabilityDetector::GetAdapterInfo(ID3D12Device* device, DirectMLCapabilities& caps) {
        try {
            ComPtr<IDXGIDevice> dxgiDevice;
            ComPtr<IDXGIAdapter> adapter;
            ComPtr<IDXGIAdapter3> adapter3;

            // Get the DXGI adapter from the D3D12 device
            HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
            if (FAILED(hr)) return;

            hr = dxgiDevice->GetAdapter(&adapter);
            if (FAILED(hr)) return;

            hr = adapter->QueryInterface(IID_PPV_ARGS(&adapter3));
            if (FAILED(hr)) return;

            // Get adapter description
            DXGI_ADAPTER_DESC2 desc;
            hr = adapter3->GetDesc2(&desc);
            if (SUCCEEDED(hr)) {
                // Convert wide string to string
                std::wstring wideDesc(desc.Description);
                caps.adapterDescription = std::string(wideDesc.begin(), wideDesc.end());

                caps.dedicatedVideoMemory = desc.DedicatedVideoMemory;
                caps.dedicatedSystemMemory = desc.DedicatedSystemMemory;
                caps.sharedSystemMemory = desc.SharedSystemMemory;

                AGK_INFO("GPU Adapter: {}", caps.adapterDescription);
                AGK_INFO("Dedicated VRAM: {} MB", caps.dedicatedVideoMemory / (1024 * 1024));
            }
        }
        catch (const std::exception& e) {
            AGK_ERROR("Exception getting adapter info: {}", e.what());
        }
    }

    std::vector<std::string> DirectMLCapabilityDetector::GetRecommendedONNXProviders(const DirectMLCapabilities& caps) {
        std::vector<std::string> providers;

        if (caps.isGPUAccelerationRecommended) {
            providers.push_back("DML");  // DirectML provider
        }

        // Always add CPU as fallback
        providers.push_back("CPU");

        return providers;
    }

    void DirectMLCapabilityDetector::LogCapabilities(const DirectMLCapabilities& caps) {
        AGK_INFO("=== DirectML Capability Detection Results ===");
        AGK_INFO("DirectML Supported: {}", caps.isDirectMLSupported ? "Yes" : "No");
        AGK_INFO("MetaCommands Supported: {}", caps.areMetaCommandsSupported ? "Yes" : "No");
        AGK_INFO("GPU Acceleration Recommended: {}", caps.isGPUAccelerationRecommended ? "Yes" : "No");
        AGK_INFO("Recommended Execution Provider: {}", caps.recommendedExecutionProvider);

        if (!caps.detectionError.empty()) {
            AGK_WARN("Detection Issue: {}", caps.detectionError);
        }

        if (!caps.adapterDescription.empty()) {
            AGK_INFO("Graphics Adapter: {}", caps.adapterDescription);
            AGK_INFO("Dedicated VRAM: {} MB", caps.dedicatedVideoMemory / (1024 * 1024));
        }

        AGK_INFO("=== End Capability Detection ===");
    }

} // namespace Angaraka::AI