#pragma once

#include "logger.hpp"
#include <chrono>
#include <mutex>

#ifndef JTX_ENABLE_PROFILING

#define PROFILE_SCOPE(name) do {} while(0)

#else

namespace jtx::detail {

struct ScopeTimer {
    explicit ScopeTimer(const char *name)
        : m_name(name) {
        m_start = std::chrono::high_resolution_clock::now();
    }

    ~ScopeTimer() {
        const auto end = std::chrono::high_resolution_clock::now();
        double ms      = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - m_start).count();
        Logger::printTime();
        fmt::print("[TIMER] {}: {:.3f} ms\n", m_name, ms);
    }

private:
    const char *m_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

}// namespace jtx::detail

#define PROFILE_SCOPE(name) jtx::detail::ScopeTimer timer##__LINE__(name)
#endif