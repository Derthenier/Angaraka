#include "Angaraka/Asset/BundleLoader.hpp"
#include "Angaraka/Log.hpp"
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace Angaraka::Core {

    BundleLoader::BundleLoader() {
        AGK_INFO("BundleLoader: Initialized");
    }

    std::optional<AssetBundleConfig> BundleLoader::LoadBundle(const std::filesystem::path& bundleFilePath) {
        if (!std::filesystem::exists(bundleFilePath)) {
            AGK_ERROR("Bundle file not found: {}", bundleFilePath.string());
            return std::nullopt;
        }

        try {
            std::ifstream file(bundleFilePath);
            std::stringstream buffer;
            buffer << file.rdbuf();

            return ParseBundleFromYaml(buffer.str(), bundleFilePath);
        }
        catch (const std::exception& e) {
            AGK_ERROR("Failed to load bundle file {}: {}", bundleFilePath.string(), e.what());
            return std::nullopt;
        }
    }

    std::vector<AssetBundleConfig> BundleLoader::LoadAllBundles(const std::filesystem::path& bundlesDirectory) {
        std::vector<AssetBundleConfig> bundles;

        if (!std::filesystem::exists(bundlesDirectory)) {
            AGK_WARN("Bundles directory not found: {}", bundlesDirectory.string());
            return bundles;
        }

        for (const auto& entry : std::filesystem::directory_iterator(bundlesDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".yaml") {
                auto bundle = LoadBundle(entry.path());
                if (bundle.has_value()) {
                    bundles.push_back(std::move(bundle.value()));
                    AGK_INFO("Loaded bundle: {}", bundles.back().name);
                }
            }
        }

        AGK_INFO("Loaded {} bundles from {}", bundles.size(), bundlesDirectory.string());
        return bundles;
    }

    std::optional<AssetBundleConfig> BundleLoader::ParseBundleFromYaml(const String& yamlContent,
        const std::filesystem::path& filePath) {
        try {
            YAML::Node config = YAML::Load(yamlContent);

            if (!config["bundle"]) {
                AGK_ERROR("Bundle file missing 'bundle' section: {}", filePath.string());
                return std::nullopt;
            }

            AssetBundleConfig bundle;
            bundle.bundleFilePath = filePath.string();

            auto bundleNode = config["bundle"];
            bundle.name = bundleNode["name"].as<String>();
            bundle.priority = bundleNode["priority"].as<AssetPriority>(PRIORITY_MEDIUM);
            bundle.autoLoad = bundleNode["auto_load"].as<bool>(true);

            if (bundleNode["unload_strategy"]) {
                bundle.unloadStrategy = ParseUnloadStrategy(bundleNode["unload_strategy"].as<String>());
            }

            // Parse dependencies
            if (bundleNode["dependencies"]) {
                for (const auto& dep : bundleNode["dependencies"]) {
                    bundle.dependencies.push_back(dep.as<String>());
                }
            }

            // Parse assets
            if (config["assets"]) {
                for (size_t i = 0; i < config["assets"].size(); ++i) {
                    auto assetNode = config["assets"][i];

                    AssetDefinition asset;
                    asset.type = AssetDefinition::StringToAssetType(assetNode["type"].as<String>());
                    asset.id = assetNode["id"].as<String>();
                    asset.path = assetNode["path"].as<String>();
                    asset.priority = assetNode["priority"].as<AssetPriority>(bundle.priority);

                    if (assetNode["unload_strategy"]) {
                        asset.unloadStrategy = ParseUnloadStrategy(assetNode["unload_strategy"].as<String>());
                    }
                    else {
                        asset.unloadStrategy = bundle.unloadStrategy;
                    }

                    bundle.assets.push_back(std::move(asset));
                }
            }

            bundle.totalAssets = bundle.assets.size();
            return bundle;
        }
        catch (const YAML::Exception& e) {
            AGK_ERROR("YAML parsing error in {}: {}", filePath.string(), e.what());
            return std::nullopt;
        }
    }

    UnloadStrategy BundleLoader::ParseUnloadStrategy(const String& strategyStr) {
        if (strategyStr == "manual") {
            return UnloadStrategy::Manual;
        }
        return UnloadStrategy::Automatic;
    }

    bool BundleLoader::ValidateBundle(const AssetBundleConfig& bundle, String& errorMessage) {
        if (bundle.name.empty()) {
            errorMessage = "Bundle name cannot be empty";
            return false;
        }

        if (bundle.assets.empty()) {
            errorMessage = "Bundle must contain at least one asset";
            return false;
        }

        // Check for duplicate asset IDs
        std::unordered_set<String> assetIds;
        for (const auto& asset : bundle.assets) {
            if (asset.id.empty()) {
                errorMessage = "Asset ID cannot be empty";
                return false;
            }

            if (assetIds.count(asset.id)) {
                errorMessage = "Duplicate asset ID: " + asset.id;
                return false;
            }
            assetIds.insert(asset.id);

            if (asset.type == AssetType::Unknown) {
                errorMessage = "Unknown asset type for asset: " + asset.id;
                return false;
            }
        }

        return true;
    }

}