#pragma once

#include "Angaraka/Base.hpp"

namespace Angaraka::Core {

    // Priority levels: 0 = highest priority, 100+ = lowest priority
    using AssetPriority = U32;

    constexpr AssetPriority PRIORITY_CRITICAL = 0;
    constexpr AssetPriority PRIORITY_HIGH = 25;
    constexpr AssetPriority PRIORITY_MEDIUM = 50;
    constexpr AssetPriority PRIORITY_LOW = 100;

    enum class AssetType : U8 {
        Texture = 0,
        Mesh,
        Material,
        Sound,
        Animation,
        Video,
        Scripts,
        Unknown,

        Count
    };

    enum class UnloadStrategy {
        Automatic,  // Engine manages unloading based on usage
        Manual      // Must be explicitly unloaded
    };

    struct AssetDefinition {
        AssetType type = AssetType::Unknown;
        String id;           // Unique identifier (e.g., "ui/button_normal")
        String path;         // File path relative to assets folder
        AssetPriority priority = PRIORITY_MEDIUM;
        UnloadStrategy unloadStrategy = UnloadStrategy::Automatic;

        // Helper to convert string to AssetType
        static AssetType StringToAssetType(const String& typeStr);
        static String AssetTypeToString(AssetType type);
    };

    struct AssetBundleConfig {
        String name;                                    // Bundle name (e.g., "UI_Elements")
        AssetPriority priority = PRIORITY_MEDIUM;           // Overall bundle priority
        bool autoLoad = true;                               // Load automatically on game start
        UnloadStrategy unloadStrategy = UnloadStrategy::Automatic;

        std::vector<String> dependencies;              // Other bundles this depends on
        std::vector<AssetDefinition> assets;               // Assets in this bundle

        // Computed fields (filled during loading)
        String bundleFilePath;                        // Path to the bundle definition file
        bool isLoaded = false;                             // Current load state
        size_t totalAssets = 0;                           // Total number of assets
        size_t loadedAssets = 0;                          // Number of successfully loaded assets

        // Helper methods
        bool HasDependency(const String& bundleName) const;
        std::vector<AssetDefinition> GetAssetsByPriority() const;  // Returns assets sorted by priority
        F32 GetLoadProgress() const;                             // Returns 0.0-1.0 load progress
    };

} // namespace Angaraka::Core