// DirectMLCapabilityDetector.hpp
// Detects DirectML and MetaCommand support for graceful fallback
#pragma once

#include <d3d12.h>
#include <dml_provider_factory.h>
#include <wrl/client.h>
#include <vector>
#include <string>

namespace Angaraka::AI {

    struct DirectMLCapabilities {
        bool isDirectMLSupported = false;
        bool areMetaCommandsSupported = false;
        bool isGPUAccelerationRecommended = false;
        std::string recommendedExecutionProvider;
        std::string detectionError;

        // Hardware info
        std::string adapterDescription;
        size_t dedicatedVideoMemory = 0;
        size_t dedicatedSystemMemory = 0;
        size_t sharedSystemMemory = 0;
    };

    class DirectMLCapabilityDetector {
    public:
        static DirectMLCapabilities DetectCapabilities(ID3D12Device* device);
        static std::vector<std::string> GetRecommendedONNXProviders(const DirectMLCapabilities& caps);
        static void LogCapabilities(const DirectMLCapabilities& caps);

    private:
        static bool TestMetaCommandSupport(ID3D12Device* device);
        static bool TestDirectMLDevice(ID3D12Device* device);
        static void GetAdapterInfo(ID3D12Device* device, DirectMLCapabilities& caps);
    };

} // namespace Angaraka::AI