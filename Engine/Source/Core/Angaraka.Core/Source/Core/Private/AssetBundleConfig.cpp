#include "Angaraka/AssetBundleConfig.hpp"
#include <algorithm>
#include <unordered_map>

namespace Angaraka::Core {

    AssetType AssetDefinition::StringToAssetType(const String& typeStr) {
        static const std::unordered_map<String, AssetType> typeMap = {
            {"texture", AssetType::Texture},
            {"mesh", AssetType::Mesh},
            {"material", AssetType::Material},
            {"sound", AssetType::Sound}
        };

        auto it = typeMap.find(typeStr);
        return (it != typeMap.end()) ? it->second : AssetType::Unknown;
    }

    String AssetDefinition::AssetTypeToString(AssetType type) {
        switch (type) {
        case AssetType::Texture: return "texture";
        case AssetType::Mesh: return "mesh";
        case AssetType::Material: return "material";
        case AssetType::Sound: return "sound";
        case AssetType::Animation: return "animation";
        case AssetType::Scripts: return "scripts";
        case AssetType::Video: return "video";
        default: return "unknown";
        }
    }

    bool AssetBundleConfig::HasDependency(const String& bundleName) const {
        return std::find(dependencies.begin(), dependencies.end(), bundleName) != dependencies.end();
    }

    std::vector<AssetDefinition> AssetBundleConfig::GetAssetsByPriority() const {
        std::vector<AssetDefinition> sortedAssets = assets;

        // Sort by priority (lower numbers = higher priority)
        std::sort(sortedAssets.begin(), sortedAssets.end(),
            [](const AssetDefinition& a, const AssetDefinition& b) {
                return a.priority < b.priority;
            });

        return sortedAssets;
    }

    F32 AssetBundleConfig::GetLoadProgress() const {
        if (totalAssets == 0) return 1.0f;
        return static_cast<F32>(loadedAssets) / static_cast<F32>(totalAssets);
    }

}