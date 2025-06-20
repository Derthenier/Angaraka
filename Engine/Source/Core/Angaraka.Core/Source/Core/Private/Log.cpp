#include <Angaraka/Log.hpp> // Include our public header
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex> // For thread-safe logging

import Angaraka.Core.Config;

namespace Angaraka::Logger
{
    std::shared_ptr<spdlog::logger> Framework::s_coreLogger = nullptr;
    std::shared_ptr<spdlog::logger> Framework::s_appLogger = nullptr;
    std::once_flag Framework::s_InitFlag;

    namespace {
        std::mutex s_LogMutex; // Protects console output
    }

    void Framework::Initialize() {
        Angaraka::Config::EngineConfig config{ Angaraka::Config::ConfigManager::GetConfig() };

        std::call_once(s_InitFlag, [&] {
            spdlog::sink_ptr engineSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.logging.engine, true);
            engineSink->set_pattern(std::format("[%T] [%8!l] [{}]: %v", config.engineName));

            s_coreLogger = spdlog::get("engine");
            if (not s_coreLogger)
            {
                s_coreLogger = std::make_shared<spdlog::logger>("engine", engineSink);
                spdlog::register_logger(s_coreLogger);

                if (config.logging.level == "trace") {
                    s_coreLogger->set_level(spdlog::level::trace);
                    s_coreLogger->flush_on(spdlog::level::trace);
                }
                else if (config.logging.level == "debug") {
                    s_coreLogger->set_level(spdlog::level::debug);
                    s_coreLogger->flush_on(spdlog::level::debug);
                }
                else if (config.logging.level == "info") {
                    s_coreLogger->set_level(spdlog::level::info);
                    s_coreLogger->flush_on(spdlog::level::info);
                }
                else if (config.logging.level == "warn") {
                    s_coreLogger->set_level(spdlog::level::warn);
                    s_coreLogger->flush_on(spdlog::level::warn);
                }
                else if (config.logging.level == "error") {
                    s_coreLogger->set_level(spdlog::level::err);
                    s_coreLogger->flush_on(spdlog::level::err);
                }
                else if (config.logging.level == "critical") {
                    s_coreLogger->set_level(spdlog::level::critical);
                    s_coreLogger->flush_on(spdlog::level::critical);
                }
                else {
                    s_coreLogger->set_level(spdlog::level::off);
                    s_coreLogger->flush_on(spdlog::level::off);
                }
            }

            spdlog::sink_ptr gameSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.logging.game, true);
            gameSink->set_pattern(std::format("[%T] [%8!l] [{}]: %v", config.gameName));

            s_appLogger = spdlog::get("game");
            if (not s_appLogger)
            {
                s_appLogger = std::make_shared<spdlog::logger>("game", gameSink);
                spdlog::register_logger(s_appLogger);

                if (config.logging.level == "trace") {
                    s_appLogger->set_level(spdlog::level::trace);
                    s_appLogger->flush_on(spdlog::level::trace);
                }
                else if (config.logging.level == "debug") {
                    s_appLogger->set_level(spdlog::level::debug);
                    s_appLogger->flush_on(spdlog::level::debug);
                }
                else if (config.logging.level == "info") {
                    s_appLogger->set_level(spdlog::level::info);
                    s_appLogger->flush_on(spdlog::level::info);
                }
                else if (config.logging.level == "warn") {
                    s_appLogger->set_level(spdlog::level::warn);
                    s_appLogger->flush_on(spdlog::level::warn);
                }
                else if (config.logging.level == "error") {
                    s_appLogger->set_level(spdlog::level::err);
                    s_appLogger->flush_on(spdlog::level::err);
                }
                else if (config.logging.level == "critical") {
                    s_appLogger->set_level(spdlog::level::critical);
                    s_appLogger->flush_on(spdlog::level::critical);
                }
                else {
                    s_appLogger->set_level(spdlog::level::off);
                    s_appLogger->flush_on(spdlog::level::off);
                }
            }
        });

        // In a real engine, this would set up file logging,
        // potentially connect to a debug console, etc.
        AGK_INFO("Angaraka Logging System Initialized.");
    }

    void Framework::Shutdown() {
        // Clean up logging resources here
        AGK_INFO("Angaraka Logging System Shut down.");

        for (long i{ 0 }; i < s_coreLogger.use_count(); ++i)
        {
            s_coreLogger.reset();
        }
    }
} // namespace Angaraka