#pragma once

#include "Angaraka/AssetBundleConfig.hpp"
#include <optional>
#include <string>
#include <vector>
#include <filesystem>

namespace Angaraka::Core {

    class BundleLoader {
    public:
        BundleLoader();
        ~BundleLoader() = default;

        // Load a single bundle from file
        std::optional<AssetBundleConfig> LoadBundle(const std::filesystem::path& bundleFilePath);

        // Load all bundles from directory
        std::vector<AssetBundleConfig> LoadAllBundles(const std::filesystem::path& bundlesDirectory);

        // Validate bundle configuration
        bool ValidateBundle(const AssetBundleConfig& bundle, String& errorMessage);

    private:
        // Parse bundle from YAML node
        std::optional<AssetBundleConfig> ParseBundleFromYaml(const String& yamlContent,
            const std::filesystem::path& filePath);

        // Parse individual asset definition
        std::optional<AssetDefinition> ParseAssetDefinition(const String& yamlContent, size_t assetIndex);

        // Helper to parse unload strategy from string
        UnloadStrategy ParseUnloadStrategy(const String& strategyStr);
    };

}