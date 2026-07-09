#pragma once

#include <cstdio>
#include <chrono>

namespace arb {
namespace log {

// A fast, synchronous logger using printf instead of std::cout (which is notoriously slow due to mutex locks).
// In Phase 5, we will upgrade this to push to a lock-free ring buffer for a background thread to write to disk.
template <typename... Args>
inline void info(const char* format, Args... args) noexcept {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    std::printf("[%lld] [INFO] ", ms);
    std::printf(format, args...);
    std::printf("\n");
}

template <typename... Args>
inline void error(const char* format, Args... args) noexcept {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    std::printf("[%lld] [ERROR] ", ms);
    std::printf(format, args...);
    std::printf("\n");
}

} // namespace log
} // namespace arb
