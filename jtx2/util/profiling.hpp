#pragma once

#include "logger.hpp"
#include <chrono>
#include <mutex>

#ifndef JTX_ENABLE_PROFILING

#define PROFILE_SCOPE(name) do {} while(0)

#define PROFILE_LOCAL_START(name) do {} while(0)
#define PROFILE_LOCAL_LOG_TIME_MILLIS() do {} while(0)
#define PROFILE_LOCAL_LOG_TIME_SECONDS() do {} while(0)
#define PROFILE_LOCAL_LOG_TIME() do {} while(0)

#define TPROFILE_SCOPE() do {} while(0)
#define TPROFILE_SCOPE_N(__name) do {} while(0)
#define TPROFILE_FRAME_MARK() do {} while(0)

#else

#include <tracy/Tracy.hpp>

namespace jtx::detail {

struct ScopeTimer {
    explicit ScopeTimer(const char *name)
        : m_name(name) {
        m_start = std::chrono::high_resolution_clock::now();
    }

    ~ScopeTimer() {
        const auto end  = std::chrono::high_resolution_clock::now();
        const double ms      = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - m_start).count();
        const auto time = Logger::Get().GetTime();
        fmt::print(fg(fmt::terminal_color::bright_green), "[JTX] [{:%M:%S}] [TIMR] {}: {} ms\n", time, m_name, ms);
    }

private:
    const char *m_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

struct Timer {
    Timer() = default;
    ~Timer() = default;

    void Start(const std::string &name) {
        m_name = name;
        m_start = Clock::now();
    }

    void LogElapsedTimeMillis() {
        const auto end = Clock::now();
        const double ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - m_start).count();
        LOG_INFO(TIMER, "{}: {} ms", m_name, ms);
    }

    void LogElapsedTimeSeconds() {
        const auto end = Clock::now();
        const double ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - m_start).count();
        LOG_INFO(TIMER, "{}: {:.3f} seconds", m_name, ms / 1000.0);
    }

    void LogElapsedTime() {
        const auto end = Clock::now();
        auto duration  = end - m_start;

        const auto hrs  = std::chrono::duration_cast<std::chrono::hours>(duration);
        duration -= hrs;
        const auto mins = std::chrono::duration_cast<std::chrono::minutes>(duration);
        duration -= mins;
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration);
        duration -= secs;
        auto ms   = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        LOG_INFO(TIMER, "{}: {}:{:02}:{:02}.{:03}", m_name, hrs.count(), mins.count(), secs.count(), ms);
    }

private:
    using Clock = std::chrono::high_resolution_clock;

    std::string m_name;
    Clock::time_point m_start;
};

}// namespace jtx::detail

#define PROFILE_SCOPE(name) jtx::detail::ScopeTimer timer##__LINE__(name)

#define PROFILE_LOCAL_START(name) jtx::detail::Timer timer##__LINE__; timer##__LINE__.Start(name)
#define PROFILE_LOCAL_LOG_TIME_MILLIS() timer##__LINE__.LogElapsedTimeMillis()
#define PROFILE_LOCAL_LOG_TIME_SECONDS() timer##__LINE__.LogElapsedTimeSeconds()
#define PROFILE_LOCAL_LOG_TIME() timer##__LINE__.LogElapsedTime()

#define TPROFILE_SCOPE() ZoneScoped
#define TPROFILE_SCOPE_N(__name) ZoneScopedN(__name)
#define TPROFILE_FRAME_MARK() FrameMark

#endif