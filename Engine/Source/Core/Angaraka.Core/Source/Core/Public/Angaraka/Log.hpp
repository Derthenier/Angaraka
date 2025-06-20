#ifndef ANGARAKA_CORE_LOG_HPP
#define ANGARAKA_CORE_LOG_HPP

#include <string>
#include <iostream>
#include <format>
#include <source_location>

#pragma warning(push, 0)
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#pragma warning(pop)

namespace Angaraka::Logger
{
    // Simple logging levels
    enum class LogLevel : uint8_t
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 4,
        Error = 8,
        Fatal = 16,
    };

    class Framework {
    public:

        // Function to initialize the logging system (e.g., setup file output)
        static void Initialize();

        // Function to shutdown the logging system
        static void Shutdown();

        // Core logging function
        template <typename... Args>
        static void Log(LogLevel level, const std::string& message, Args ... args) {
            if (s_coreLogger) {
                switch (level) {
                case LogLevel::Trace: s_coreLogger->trace(fmt::runtime(message),    std::forward<Args>(args)...); break;
                case LogLevel::Debug: s_coreLogger->debug(fmt::runtime(message),    std::forward<Args>(args)...); break;
                case LogLevel::Info:  s_coreLogger->info(fmt::runtime(message),     std::forward<Args>(args)...); break;
                case LogLevel::Warn:  s_coreLogger->warn(fmt::runtime(message),     std::forward<Args>(args)...); break;
                case LogLevel::Error: s_coreLogger->error(fmt::runtime(message),    std::forward<Args>(args)...); break;
                case LogLevel::Fatal: s_coreLogger->critical(fmt::runtime(message), std::forward<Args>(args)...);  break;
                };
            }
        }
        template <typename... Args>
        static void Log(LogLevel level, const std::wstring& message, Args ... args) {
            if (s_coreLogger) {
                switch (level) {
                case LogLevel::Trace: s_coreLogger->trace(fmt::runtime(message),    std::forward<Args>(args)...); break;
                case LogLevel::Debug: s_coreLogger->debug(fmt::runtime(message),    std::forward<Args>(args)...); break;
                case LogLevel::Info:  s_coreLogger->info(fmt::runtime(message),     std::forward<Args>(args)...); break;
                case LogLevel::Warn:  s_coreLogger->warn(fmt::runtime(message),     std::forward<Args>(args)...); break;
                case LogLevel::Error: s_coreLogger->error(fmt::runtime(message),    std::forward<Args>(args)...); break;
                case LogLevel::Fatal: s_coreLogger->critical(fmt::runtime(message), std::forward<Args>(args)...);  break;
                };
            }
        }

        // App logging function
        template <typename... Args>
        static void LogApp(LogLevel level, const std::string& message, Args ... args) {
            if (s_coreLogger) {
                switch (level) {
                case LogLevel::Trace: s_appLogger->trace(fmt::runtime(message), std::forward<Args>(args)...); break;
                case LogLevel::Debug: s_appLogger->debug(fmt::runtime(message), std::forward<Args>(args)...); break;
                case LogLevel::Info:  s_appLogger->info(fmt::runtime(message), std::forward<Args>(args)...); break;
                case LogLevel::Warn:  s_appLogger->warn(fmt::runtime(message), std::forward<Args>(args)...); break;
                case LogLevel::Error: s_appLogger->error(fmt::runtime(message), std::forward<Args>(args)...); break;
                case LogLevel::Fatal: s_appLogger->critical(fmt::runtime(message), std::forward<Args>(args)...);  break;
                };
            }
        }
        template <typename... Args>
        static void LogApp(LogLevel level, const std::wstring& message, Args ... args) {
            if (s_coreLogger) {
                switch (level) {
                case LogLevel::Trace: s_appLogger->trace(fmt::runtime(message), std::forward<Args>(args)...); break;
                case LogLevel::Debug: s_appLogger->debug(fmt::runtime(message), std::forward<Args>(args)...); break;
                case LogLevel::Info:  s_appLogger->info(fmt::runtime(message), std::forward<Args>(args)...); break;
                case LogLevel::Warn:  s_appLogger->warn(fmt::runtime(message), std::forward<Args>(args)...); break;
                case LogLevel::Error: s_appLogger->error(fmt::runtime(message), std::forward<Args>(args)...); break;
                case LogLevel::Fatal: s_appLogger->critical(fmt::runtime(message), std::forward<Args>(args)...);  break;
                };
            }
        }

    private:
        static std::shared_ptr<spdlog::logger> s_coreLogger;
        static std::shared_ptr<spdlog::logger> s_appLogger;
        static std::once_flag s_InitFlag;
    };

// Convenience macros for easier logging
#define AGK_TRACE(...) ::Angaraka::Logger::Framework::Log(Angaraka::Logger::LogLevel::Trace, __VA_ARGS__)
#define AGK_DEBUG(...) ::Angaraka::Logger::Framework::Log(Angaraka::Logger::LogLevel::Debug, __VA_ARGS__)
#define AGK_INFO(...)  ::Angaraka::Logger::Framework::Log(Angaraka::Logger::LogLevel::Info,  __VA_ARGS__)
#define AGK_WARN(...)  ::Angaraka::Logger::Framework::Log(Angaraka::Logger::LogLevel::Warn,  __VA_ARGS__)
#define AGK_ERROR(...) ::Angaraka::Logger::Framework::Log(Angaraka::Logger::LogLevel::Error, __VA_ARGS__)
#define AGK_FATAL(...) ::Angaraka::Logger::Framework::Log(Angaraka::Logger::LogLevel::Fatal, __VA_ARGS__)


// Convenience macros for easier logging
#define AGK_APP_TRACE(...) ::Angaraka::Logger::Framework::LogApp(Angaraka::Logger::LogLevel::Trace, __VA_ARGS__)
#define AGK_APP_DEBUG(...) ::Angaraka::Logger::Framework::LogApp(Angaraka::Logger::LogLevel::Debug, __VA_ARGS__)
#define AGK_APP_INFO(...)  ::Angaraka::Logger::Framework::LogApp(Angaraka::Logger::LogLevel::Info,  __VA_ARGS__)
#define AGK_APP_WARN(...)  ::Angaraka::Logger::Framework::LogApp(Angaraka::Logger::LogLevel::Warn,  __VA_ARGS__)
#define AGK_APP_ERROR(...) ::Angaraka::Logger::Framework::LogApp(Angaraka::Logger::LogLevel::Error, __VA_ARGS__)
#define AGK_APP_FATAL(...) ::Angaraka::Logger::Framework::LogApp(Angaraka::Logger::LogLevel::Fatal, __VA_ARGS__)

} // namespace Angaraka

#endif // ANGARAKA_CORE_LOG_HPP