#pragma once

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

    static void printTime() {
        const std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        fmt::print("[JTX] [{:%M:%S}] ", end - Logger::get().start);
    }

    template<typename... Args>
    static void print(fmt::format_string<Args...> format, Args &&...args) {
        fmt::print(format, std::forward<Args>(args)...);
        fmt::print("\n");
    }

    template<typename... Args>
    static void log(const LogContext ctx, fmt::format_string<Args...> format, Args &&...args) {
        printTime();

        switch (ctx.category) {
            case LogCategory::GENERAL:
                fmt::print("[GENERAL] ");
                break;
            case LogCategory::DISPLAY:
                fmt::print("[DISPLAY] ");
                break;
            case LogCategory::UI:
                fmt::print("[UI] ");
                break;
            case LogCategory::RASTERIZER:
                fmt::print("[RASTERIZER] ");
                break;
            case LogCategory::TRACER:
                fmt::print("[TRACER] ");
                break;
            case LogCategory::VULKAN:
                fmt::print("[VULKAN] ");
                break;
            case LogCategory::TEXTURE:
                fmt::print("[TEXTURE] ");
                break;
            case LogCategory::LOADER:
                fmt::print("[LOADER] ");
                break;
            case LogCategory::INPUT:
                fmt::print("[INPUT] ");
                break;
        }

        switch (ctx.level) {
            case LogLevel::INFO:
                fmt::print("[INFO] ");
                break;
            case LogLevel::ERR:
                fmt::print("[ERROR] ");
                break;
            case LogLevel::DEBUG:
                fmt::print("[DEBUG] ");
                break;
            case LogLevel::FATAL:
                fmt::print("[FATAL] ");
                break;
        }

        fmt::print("[{}:{}] ", ctx.function, ctx.line);

        print(format, std::forward<Args>(args)...);
        if (ctx.level == LogLevel::FATAL) {
            abort();
        }
    }
};