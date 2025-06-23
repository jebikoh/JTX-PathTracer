#pragma once

#include <chrono>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <functional>

#if defined(NDEBUG)
#define LOG_DEBUG(category, msg, ...) \
    do { /* nothing */                \
    } while (0)
#else
#define LOG_DEBUG(category, msg, ...) \
    Logger::Get().Log({__LINE__, __func__, LogLevel::DEBUG, LogCategory::category}, msg, ##__VA_ARGS__)
#endif

#define LOG_INFO(category, msg, ...) \
    Logger::Get().Log({__LINE__, __func__, LogLevel::INFO, LogCategory::category}, msg, ##__VA_ARGS__)

#define LOG_ERROR(category, msg, ...) \
    Logger::Get().Log({__LINE__, __func__, LogLevel::ERR, LogCategory::category}, msg, ##__VA_ARGS__)

#define LOG_FATAL(category, msg, ...) \
    Logger::Get().Log({__LINE__, __func__, LogLevel::FATAL, LogCategory::category}, msg, ##__VA_ARGS__)

enum class LogLevel {
    INFO  = 0,
    DEBUG = 1,
    ERR   = 2,// I hate windows so much...
    FATAL = 3,
};

enum class LogCategory {
    GENERAL = 0,
    DISPLAY = 1,
    UI      = 2,
    WING    = 3,
    TRACER  = 4,
    VULKAN  = 5,
    TEXTURE = 6,
    LOADER  = 7,
    INPUT   = 8,
    TEST    = 9,
    RENDER  = 10,
    TIMER   = 11,
};

struct LogContext {
    int line;
    const char *function;
    LogLevel level;
    LogCategory category;
};

enum class LogColor {
    WHITE,
    YELLOW,
    BLUE,
    RED,
};

struct LogEntry {
    LogColor color;
    std::string message;
};

struct Logger {
    using Sink = std::function<void(const LogEntry &)>;

    void AddSink(const Sink &fn) { m_sinks.push_back(fn); }
    void ClearSinks() { m_sinks.clear(); }

    static Logger &Get() {
        static Logger logger;
        return logger;
    }

    template<typename... Args>
    void Log(const LogContext ctx, fmt::format_string<Args...> format, Args &&...args) {
        const auto entry = FormatMessage(ctx, format, std::forward<Args>(args)...);
        for (auto &sink: m_sinks) {
            sink(entry);
        }

        if (ctx.level == LogLevel::FATAL) abort();
    }

    auto GetTime() const {
        const std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        return end - m_start;
    }

    /**
     * Adds a default sink that prints log messages to stdout.
     */
    static void AddDefaultSink() {
        Logger::Get().AddSink([](LogEntry const &entry) {
            fmt::text_style color;
            switch (entry.color) {
                case LogColor::YELLOW:
                    color = fg(fmt::terminal_color::yellow);
                    break;
                case LogColor::BLUE:
                    color = fg(fmt::terminal_color::blue);
                    break;
                case LogColor::RED:
                    color = fg(fmt::terminal_color::bright_red);
                    break;
                default:
                    break;
            }
            fmt::print(color, "{}\n", entry.message);
        });
    }

private:
    std::chrono::time_point<std::chrono::system_clock> m_start;
    std::vector<Sink> m_sinks{};

    Logger()
        : m_start(std::chrono::system_clock::now()) {}

    template<typename... Args>
    static auto FormatMessage(const LogContext ctx, fmt::format_string<Args...> format, Args &&...args) {
        auto buf = fmt::memory_buffer();

        auto color = LogColor::WHITE;
        switch (ctx.level) {
            case LogLevel::ERR:
                color = LogColor::YELLOW;
                break;
            case LogLevel::DEBUG:
                color = LogColor::BLUE;
                break;
            case LogLevel::FATAL:
                color = LogColor::RED;
                break;
            default:
                break;
        }

        // Time
        const std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        fmt::format_to(std::back_inserter(buf), "[JTX] [{:%M:%S}] ", end - Logger::Get().m_start);

        // Category
        switch (ctx.category) {
            case LogCategory::GENERAL:
                fmt::format_to(std::back_inserter(buf), "[GNRL] ");
                break;
            case LogCategory::DISPLAY:
                fmt::format_to(std::back_inserter(buf), "[DISP] ");
                break;
            case LogCategory::UI:
                fmt::format_to(std::back_inserter(buf), "[UIUX] ");
                break;
            case LogCategory::WING:
                fmt::format_to(std::back_inserter(buf), "[WING] ");
                break;
            case LogCategory::TRACER:
                fmt::format_to(std::back_inserter(buf), "[TRCR] ");
                break;
            case LogCategory::VULKAN:
                fmt::format_to(std::back_inserter(buf), "[VLKN] ");
                break;
            case LogCategory::TEXTURE:
                fmt::format_to(std::back_inserter(buf), "[TXTR] ");
                break;
            case LogCategory::LOADER:
                fmt::format_to(std::back_inserter(buf), "[LOAD] ");
                break;
            case LogCategory::INPUT:
                fmt::format_to(std::back_inserter(buf), "[INPT] ");
                break;
            case LogCategory::TEST:
                fmt::format_to(std::back_inserter(buf), "[TEST] ");
                break;
            case LogCategory::RENDER:
                fmt::format_to(std::back_inserter(buf), "[RNDR] ");
                break;
            case LogCategory::TIMER:
                fmt::format_to(std::back_inserter(buf), "[TIMR] ");
        }

        // Level
        switch (ctx.level) {
            case LogLevel::INFO:
                fmt::format_to(std::back_inserter(buf), "[INF] ");
                break;
            case LogLevel::ERR:
                fmt::format_to(std::back_inserter(buf), "[ERR] [{}:{}] ", ctx.function, ctx.line);
                break;
            case LogLevel::DEBUG:
                fmt::format_to(std::back_inserter(buf), "[DBG] [{}:{}] ", ctx.function, ctx.line);
                break;
            case LogLevel::FATAL:
                fmt::format_to(std::back_inserter(buf), "[FTL] [{}:{}] ", ctx.function, ctx.line);
                break;
        }

        // Message
        fmt::format_to(std::back_inserter(buf), format, std::forward<Args>(args)...);

        return LogEntry{
                .color   = color,
                .message = to_string(buf)};
    }
};