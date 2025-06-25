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
        String name;
        String type;
        std::filesystem::path path;
    };

    export struct LogConfig {
        String level{ "info" };
        String engine{ "Angaraka.log" };
        String game{ "Angaraka.log" };
    };

    export struct WindowConfig {
        int width{ 1280 };
        int height{ 720 };
        String title{ "Angaraka Engine" };
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
        F32 clearRed{ 0.0f };
        F32 clearGreen{ 0.0f };
        F32 clearBlue{0.0f};
        String shaderCachePath{ "shaders/cache" }; // Path to the shader cache directory
        ResourceCacheConfig resourceCache;
    };

    // AI system initialization configuration
    export struct AISystemConfig {
        bool enableGPUAcceleration{ true };
        size_t maxVRAMUsageMB{ 16384 };        // 16GB default for RTX 4080/4090
        F32 dialogueTimeoutMs{ 100.0f };     // Max time for dialogue inference
        F32 terrainTimeoutMs{ 5000.0f };     // Max time for terrain generation
        size_t backgroundThreadCount{ 4 };     // Threads for async operations
        String defaultFaction{ "neutral" };
        bool enablePerformanceMonitoring{ true };
    };

    export struct EngineConfig {
        String engineName;
        String engineVersion;
        String gameName;
        String assetsBasePath{ "Assets" };
        String shadersBasePath{ "Shaders" };

        std::vector<PluginInfo> plugins;
        std::vector<String> pluginPaths;

        LogConfig logging;
        WindowConfig window;
        RendererConfig renderer;
        AISystemConfig ai;

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
                    info.name = node["name"].as<String>("");
                    info.type = node["type"].as<String>("");
                    if (node["path"])
                        info.path = node["path"].as<String>();
                    if (!info.name.empty() && !info.type.empty())
                        plugins.push_back(std::move(info));
                }
            }
            return plugins;
        }

        static std::optional<EngineConfig> LoadConfig(const String& filename)
        {
            EngineConfig ec;
            try {
                YAML::Node config = YAML::LoadFile(filename.c_str());

                // Parse engine section
                if (auto engineNode = config["engine"]) {
                    if (engineNode["name"])
                        ec.engineName = engineNode["name"].as<String>();
                    if (engineNode["version"])
                        ec.engineVersion = engineNode["version"].as<String>();
                    if (engineNode["game"])
                        ec.gameName = engineNode["game"].as<String>();
                    if (engineNode["assets"])
                        ec.assetsBasePath = engineNode["assets"].as<String>();
                    if (engineNode["shaders"])
                        ec.shadersBasePath = engineNode["shaders"].as<String>();
                }

                // plugins list
                if (auto pluginsNode = config["plugins"]) {
                    ec.plugins = ParsePlugins(pluginsNode);
                }

                // Parse plugin_paths
                if (auto pathsNode = config["plugin_paths"]) {
                    for (auto it = pathsNode.begin(); it != pathsNode.end(); ++it) {
                        ec.pluginPaths.push_back(it->as<String>());
                    }
                }

                // Parse logging
                if (auto loggingNode = config["logging"]) {
                    ec.logging = {};
                    if (loggingNode["level"])
                        ec.logging.level = loggingNode["level"].as<String>();
                    if (loggingNode["engine"])
                        ec.logging.engine = loggingNode["engine"].as<String>();
                    if (loggingNode["game"])
                        ec.logging.game = loggingNode["game"].as<String>();
                }

                // Parse window config (now at root)
                if (auto windowNode = config["window"]) {
                    ec.window = {};
                    if (windowNode["width"])
                        ec.window.width = windowNode["width"].as<int>();
                    if (windowNode["height"])
                        ec.window.height = windowNode["height"].as<int>();
                    if (windowNode["title"])
                        ec.window.title = windowNode["title"].as<String>();
                }

                // Parse ai config (now at root)
                if (auto aiNode = config["ai"]) {
                    if (auto gpuAccelNode = aiNode["enable_gpu_acceleration"])
                        ec.ai.enableGPUAcceleration = gpuAccelNode.as<bool>(true);
                    if (auto maxVramNode = aiNode["max_vram_usage_mb"])
                        ec.ai.maxVRAMUsageMB = maxVramNode.as<size_t>(16384); // Default to 16GB
                    if (auto dialogueTimeoutNode = aiNode["dialogue_timeout_ms"])
                        ec.ai.dialogueTimeoutMs = dialogueTimeoutNode.as<F32>(100.0f);
                    if (auto terrainTimeoutNode = aiNode["terrain_timeout_ms"])
                        ec.ai.terrainTimeoutMs = terrainTimeoutNode.as<F32>(5000.0f);
                    if (auto backgroundThreadCountNode = aiNode["background_thread_count"])
                        ec.ai.backgroundThreadCount = backgroundThreadCountNode.as<size_t>(4);
                    if (auto defaultFactionNode = aiNode["default_faction"])
                        ec.ai.defaultFaction = defaultFactionNode.as<String>("neutral");
                    if (auto enablePerformanceMonitoringNode = aiNode["enable_performance_monitoring"])
                        ec.ai.enablePerformanceMonitoring = enablePerformanceMonitoringNode.as<bool>(true);
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
                                ec.renderer.clearRed = clearNode["red"].as<F32>(0.1f);
                            if (clearNode["green"])
                                ec.renderer.clearGreen = clearNode["green"].as<F32>(0.1f);
                            if (clearNode["blue"])
                                ec.renderer.clearBlue = clearNode["blue"].as<F32>(0.1f);
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
                            ec.renderer.shaderCachePath = shaderNode["shader_cache_dir"].as<String>("shaders/cache");
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
