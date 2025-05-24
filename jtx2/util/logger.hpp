#pragma once

#include <chrono>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>

#if defined(NDEBUG)
#define LOG_DEBUG(category, msg, ...) \
    do { /* nothing */                \
    } while (0)
#else
#define LOG_DEBUG(category, msg, ...) \
    Logger::get().log({__LINE__, __func__, LogLevel::DEBUG, LogCategory::category}, msg, ##__VA_ARGS__)
#endif

#define LOG_INFO(category, msg, ...) \
    Logger::get().log({__LINE__, __func__, LogLevel::INFO, LogCategory::category}, msg, ##__VA_ARGS__)

#define LOG_ERROR(category, msg, ...) \
    Logger::get().log({__LINE__, __func__, LogLevel::ERR, LogCategory::category}, msg, ##__VA_ARGS__)

#define LOG_FATAL(category, msg, ...) \
    Logger::get().log({__LINE__, __func__, LogLevel::FATAL, LogCategory::category}, msg, ##__VA_ARGS__)

enum class LogLevel {
    INFO  = 0,
    DEBUG = 1,
    ERR   = 2,// I hate windows so much...
    FATAL = 3,
};

enum class LogCategory {
    GENERAL    = 0,
    DISPLAY    = 1,
    UI         = 2,
    RASTERIZER = 3,
    TRACER     = 4,
    VULKAN     = 5,
    TEXTURE    = 6,
    LOADER     = 7,
    INPUT      = 8,
    TEST       = 9,
    RENDER     = 10,
};

struct LogContext {
    int line;
    const char *function;
    LogLevel level;
    LogCategory category;
};

struct Logger {
    std::chrono::time_point<std::chrono::system_clock> start;

    Logger()
        : start(std::chrono::system_clock::now()) {}

    static Logger &get() {
        static Logger logger;
        return logger;
    }

    static void printTime(const fmt::text_style &color) {
        const std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        fmt::print(color, "[JTX] [{:%M:%S}] ", end - Logger::get().start);
    }

    template<typename... Args>
    static void print(const fmt::text_style &color, fmt::format_string<Args...> format, Args &&...args) {
        fmt::print(color, static_cast<fmt::string_view>(format), std::forward<Args>(args)...);
        fmt::print("\n");
    }

    template<typename... Args>
    static void log(const LogContext ctx, fmt::format_string<Args...> format, Args &&...args) {
        fmt::text_style color;
        switch (ctx.level) {
            case LogLevel::ERR:
                color = fg(fmt::terminal_color::yellow);
                break;
            case LogLevel::DEBUG:
                color = fg(fmt::terminal_color::blue);
                break;
            case LogLevel::FATAL:
                color = fg(fmt::terminal_color::bright_red);
                break;
            default:
                break;
        }
        printTime(color);

        switch (ctx.category) {
            case LogCategory::GENERAL:
                fmt::print(color, "[GNRL] ");
                break;
            case LogCategory::DISPLAY:
                fmt::print(color, "[DISP] ");
                break;
            case LogCategory::UI:
                fmt::print(color, "[UIUX] ");
                break;
            case LogCategory::RASTERIZER:
                fmt::print(color, "[RSTR] ");
                break;
            case LogCategory::TRACER:
                fmt::print(color, "[TRCR] ");
                break;
            case LogCategory::VULKAN:
                fmt::print(color, "[VLKN] ");
                break;
            case LogCategory::TEXTURE:
                fmt::print(color, "[TXTR] ");
                break;
            case LogCategory::LOADER:
                fmt::print(color, "[LOAD] ");
                break;
            case LogCategory::INPUT:
                fmt::print(color, "[INPT] ");
                break;
            case LogCategory::TEST:
                fmt::print(color, "[TEST] ");
                break;
            case LogCategory::RENDER:
                fmt::print(color, "[RNDR] ");
                break;
        }

        switch (ctx.level) {
            case LogLevel::INFO:
                fmt::print(color, "[INF] ");
                break;
            case LogLevel::ERR:
                fmt::print(color, "[ERR] ");
                fmt::print(color, "[{}:{}] ", ctx.function, ctx.line);
                break;
            case LogLevel::DEBUG:
                fmt::print(color, "[DBG] ");
                fmt::print(color, "[{}:{}] ", ctx.function, ctx.line);
                break;
            case LogLevel::FATAL:
                fmt::print(color, "[FTL] ");
                fmt::print(color, "[{}:{}] ", ctx.function, ctx.line);
                break;
        }

        print(color, format, std::forward<Args>(args)...);
        if (ctx.level == LogLevel::FATAL) {
            abort();
        }
    }
};