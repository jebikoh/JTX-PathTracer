#pragma once

#include <chrono>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>

#define LOG_INFO(msg, ...) Logger::get().log(LogLevel::INFO, msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) Logger::get().log(LogLevel::ERR, msg, ##__VA_ARGS__)
#define LOG_FATAL(msg, ...) Logger::get().log(LogLevel::FATAL, msg, ##__VA_ARGS__)

enum class LogLevel {
    INFO  = 0,
    ERR = 1, // I hate windows so much...
    DEBUG = 2,
    FATAL = 3
};

struct Logger {
    std::chrono::time_point<std::chrono::system_clock> start;

    Logger() : start(std::chrono::system_clock::now()) {}

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
    static void log(const LogLevel level, fmt::format_string<Args...> format, Args &&...args) {
        printTime();
        switch (level) {
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

        print(format, std::forward<Args>(args)...);
        if (level == LogLevel::FATAL) {
            abort();
        }
    }
};