#include "Angaraka/AssetBundleConfig.hpp"
#include <algorithm>
#include <unordered_map>

namespace Angaraka::Core {

    AssetType AssetDefinition::StringToAssetType(const std::string& typeStr) {
        static const std::unordered_map<std::string, AssetType> typeMap = {
            {"texture", AssetType::Texture},
            {"mesh", AssetType::Mesh},
            {"material", AssetType::Material},
            {"sound", AssetType::Sound}
        };

        auto it = typeMap.find(typeStr);
        return (it != typeMap.end()) ? it->second : AssetType::Unknown;
    }

    std::string AssetDefinition::AssetTypeToString(AssetType type) {
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

    bool AssetBundleConfig::HasDependency(const std::string& bundleName) const {
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

    float AssetBundleConfig::GetLoadProgress() const {
        if (totalAssets == 0) return 1.0f;
        return static_cast<float>(loadedAssets) / static_cast<float>(totalAssets);
    }

}