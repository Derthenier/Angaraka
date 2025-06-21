module;

#include "Angaraka/Base.hpp"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <iostream>

#ifndef ANGARAKA_CONFIGURATION_FILE
#define ANGARAKA_CONFIGURATION_FILE ANGARAKA_CONFIGURATION_FOLDER "/engine.yaml"
#endif // ANGARAKA_CONFIGURATION_FILE

export module Angaraka.Core.Config;

import Angaraka.Core.ResourceCache;

namespace Angaraka::Config {

    // Plugin info for new YAML format
    export struct PluginInfo {
        std::string name;
        std::string type;
        std::filesystem::path path;
    };

    export struct LogConfig {
        std::string level{ "info" };
        std::string engine{ "Angaraka.log" };
        std::string game{ "Angaraka.log" };
    };

    export struct WindowConfig {
        int width{ 1280 };
        int height{ 720 };
        std::string title{ "Angaraka Engine" };
        bool fullscreen{ false };
    };

    export struct ResourceCacheConfig {
        int maxMemoryMB = 512;
        int maxSingleResourceMB = 64;
        int evictionThreshold = 85;
        bool enableEviction = true;
        bool logEvictions = true;

        Core::MemoryBudget ToMemoryBudget() const {
            Core::MemoryBudget budget;
            budget.maxTotalMemory = maxMemoryMB * 1024 * 1024;
            budget.maxSingleResource = maxSingleResourceMB * 1024 * 1024;
            budget.evictionThreshold = evictionThreshold;
            budget.enableEviction = enableEviction;
            budget.logEvictions = logEvictions;
            return budget;
        }
    };

    export struct RendererConfig {
        bool vsyncEnabled{ false };
        bool msaa_enabled{ false };
        bool debugLayerEnabled{ false };
        bool tearSupportEnabled{ false };
        bool fullscreen{ false };
        bool raytracingEnabled{ false };
        bool variableRateShadingEnabled{ false };
        int msaa_sampleCount{ 1 }; // 1 for no MSAA, 2, 4, 8, etc. for MSAA levels
        int msaa_qualityLevel{ 0 }; // Quality level for MSAA, typically 0-3
        int backbufferCount{ 2 }; // Default to double buffering
        int refreshRate{ 60 }; // in Hz
        float clearRed{ 0.0f };
        float clearGreen{ 0.0f };
        float clearBlue{0.0f};
        std::string shaderCachePath{ "shaders/cache" }; // Path to the shader cache directory
        ResourceCacheConfig resourceCache;
    };

    export struct EngineConfig {
        std::string engineName;
        std::string engineVersion;
        std::string gameName;

        std::vector<PluginInfo> plugins;
        std::vector<std::string> pluginPaths;

        LogConfig logging;
        WindowConfig window;
        RendererConfig renderer;

        bool loaded = false;
    };

    export class ConfigManager {
    public:
        static void Initialize() {
            std::filesystem::path configFile{ ANGARAKA_CONFIGURATION_FILE };
            if (std::filesystem::exists(configFile)) {
                std::cout << "Loading configuration from: " << configFile << "\n";
                auto config = LoadConfig(configFile.string());
                if (config) {
                    std::cout << "Configuration loaded successfully.\n";
                    defaultConfig = *config;
                    defaultConfig.loaded = true;
                }
                else {
                    defaultConfig.engineName = "Angaraka Engine";
                    defaultConfig.engineVersion = "1.0.0";
                    defaultConfig.plugins = {};
                    defaultConfig.pluginPaths = {};
                    defaultConfig.logging = {};
                    defaultConfig.window = {};
                }
            }
            else {
                // Handle error: file does not exist
                defaultConfig.engineName = "Angaraka Engine";
                defaultConfig.engineVersion = "1.0.0";
                defaultConfig.plugins = {};
                defaultConfig.pluginPaths = {};
                defaultConfig.logging = {};
                defaultConfig.window = {};
            }
        }

        static const EngineConfig& GetConfig() {
            return defaultConfig;
        }

    private:
        static EngineConfig defaultConfig;

        // Helper: Parse plugin list from YAML
        static std::vector<PluginInfo> ParsePlugins(const YAML::Node& pluginsNode)
        {
            std::vector<PluginInfo> plugins;
            for (const auto& plugin : pluginsNode) {
                if (plugin.IsMap()) {
                    const YAML::Node& node = plugin;
                    PluginInfo info;
                    info.name = node["name"].as<std::string>("");
                    info.type = node["type"].as<std::string>("");
                    if (node["path"])
                        info.path = node["path"].as<std::string>();
                    if (!info.name.empty() && !info.type.empty())
                        plugins.push_back(std::move(info));
                }
            }
            return plugins;
        }

        static std::optional<EngineConfig> LoadConfig(const std::string& filename)
        {
            EngineConfig ec;
            try {
                YAML::Node config = YAML::LoadFile(filename.c_str());

                // Parse engine section
                if (auto engineNode = config["engine"]) {
                    if (engineNode["name"])
                        ec.engineName = engineNode["name"].as<std::string>();
                    if (engineNode["version"])
                        ec.engineVersion = engineNode["version"].as<std::string>();
                    if (engineNode["game"])
                        ec.gameName = engineNode["game"].as<std::string>();
                }

                // plugins list
                if (auto pluginsNode = config["plugins"]) {
                    ec.plugins = ParsePlugins(pluginsNode);
                }

                // Parse plugin_paths
                if (auto pathsNode = config["plugin_paths"]) {
                    for (auto it = pathsNode.begin(); it != pathsNode.end(); ++it) {
                        ec.pluginPaths.push_back(it->as<std::string>());
                    }
                }

                // Parse logging
                if (auto loggingNode = config["logging"]) {
                    ec.logging = {};
                    if (loggingNode["level"])
                        ec.logging.level = loggingNode["level"].as<std::string>();
                    if (loggingNode["engine"])
                        ec.logging.engine = loggingNode["engine"].as<std::string>();
                    if (loggingNode["game"])
                        ec.logging.game = loggingNode["game"].as<std::string>();
                }

                // Parse window config (now at root)
                if (auto windowNode = config["window"]) {
                    ec.window = {};
                    if (windowNode["width"])
                        ec.window.width = windowNode["width"].as<int>();
                    if (windowNode["height"])
                        ec.window.height = windowNode["height"].as<int>();
                    if (windowNode["title"])
                        ec.window.title = windowNode["title"].as<std::string>();
                }

                // Parse renderer config (now at root)
                if (auto rendererNode = config["renderer"]) {
                    if (auto generalNode = rendererNode["general"]) {
                        if (generalNode["vsync"])
                            ec.renderer.vsyncEnabled = generalNode["vsync"].as<bool>(true);
                        if (generalNode["backbuffer_count"])
                            ec.renderer.backbufferCount = generalNode["backbuffer_count"].as<int>(2);

                        if (auto resolutionNode = generalNode["resolution"]) {
                            if (resolutionNode["fullscreen"])
                                ec.renderer.fullscreen = resolutionNode["fullscreen"].as<bool>(false);
                            if (resolutionNode["refresh_rate"])
                                ec.renderer.refreshRate = resolutionNode["refresh_rate"].as<int>(60);
                        }
                        if (auto clearNode = generalNode["clear_color"]) {
                            if (clearNode["red"])
                                ec.renderer.clearRed = clearNode["red"].as<float>(0.1f);
                            if (clearNode["green"])
                                ec.renderer.clearGreen = clearNode["green"].as<float>(0.1f);
                            if (clearNode["blue"])
                                ec.renderer.clearBlue = clearNode["blue"].as<float>(0.1f);
                        }
                    }

                    // Parse advanced section
                    if (auto advNode = rendererNode["advanced"]) {
                        if (advNode["debug_layer"])
                            ec.renderer.debugLayerEnabled = advNode["debug_layer"].as<bool>(false);
                        if (advNode["tear_support"])
                            ec.renderer.tearSupportEnabled = advNode["tear_support"].as<bool>(false);
                    }

                    // Parse msaa section
                    if (auto msaaNode = rendererNode["msaa"]) {
                        if (msaaNode["enabled"])
                            ec.renderer.msaa_enabled = msaaNode["enabled"].as<bool>(true);
                        if (msaaNode["sample_count"])
                            ec.renderer.msaa_sampleCount = msaaNode["sample_count"].as<int>(4);
                        if (msaaNode["quality_level"])
                            ec.renderer.msaa_qualityLevel = msaaNode["quality_level"].as<int>(0);
                    }

                    // Parse shaders section
                    if (auto shaderNode = rendererNode["shaders"]) {
                        if (shaderNode["shader_cache_dir"])
                            ec.renderer.shaderCachePath = shaderNode["shader_cache_dir"].as<std::string>("shaders/cache");
                    }

                    // Parse features section
                    if (auto featuresNode = rendererNode["features"]) {
                        if (featuresNode["raytracing"])
                            ec.renderer.raytracingEnabled = featuresNode["raytracing"].as<bool>(false);
                        if (featuresNode["variable_rate_shading"])
                            ec.renderer.variableRateShadingEnabled = featuresNode["variable_rate_shading"].as<bool>(false);
                    }

                    if (auto cacheNode = rendererNode["resource_cache"]) {
                        if (cacheNode["max_memory_mb"])
                            ec.renderer.resourceCache.maxMemoryMB = cacheNode["max_memory_mb"].as<int>();
                        if (cacheNode["max_single_resource_mb"])
                            ec.renderer.resourceCache.maxSingleResourceMB = cacheNode["max_single_resource_mb"].as<int>();
                        if (cacheNode["eviction_threshold"])
                            ec.renderer.resourceCache.evictionThreshold = cacheNode["eviction_threshold"].as<int>();
                        if (cacheNode["enable_eviction"])
                            ec.renderer.resourceCache.enableEviction = cacheNode["enable_eviction"].as<bool>();
                        if (cacheNode["log_evictions"])
                            ec.renderer.resourceCache.logEvictions = cacheNode["log_evictions"].as<bool>();
                    }
                }

                return ec;
            }
            catch (const YAML::ParserException& ex) {
                // Log error
                std::cerr << "Failed to parse configuration: " << ex.what() << "\n";
                return std::nullopt;
            }
            catch (const YAML::BadFile& ex) {
                // Log error
                std::cerr << "Failed to load configuration for file: " << filename << " : " << ex.what() << "\n";
                return std::nullopt;
            }
            catch (const std::exception& e) {
                // Log error
                std::cerr << "Failed to read configuration: " << e.what() << '\n';
                return std::nullopt;
            }
        }
    };

    EngineConfig ConfigManager::defaultConfig = {}; // Initialize static member
}
