#pragma once

#include <chrono>
#include <utility>

namespace debugger {
    class Util {
    public:
        template <typename Duration = std::chrono::nanoseconds, typename Func,
            typename = std::enable_if_t<!std::is_void_v<std::invoke_result_t<Func>>>>
        static auto timeFunction(Func&& func, Duration& duration) -> decltype(func()) {
            auto start = std::chrono::high_resolution_clock::now();
            auto result = func(); 
            auto end = std::chrono::high_resolution_clock::now();

            duration = std::chrono::duration_cast<Duration>(end - start);
            return result;
        }

        template <typename Duration = std::chrono::nanoseconds, typename Func,
            typename = std::enable_if_t<std::is_void_v<std::invoke_result_t<Func>>>>
        static void timeFunction(Func&& func, Duration& duration) {
            auto start = std::chrono::high_resolution_clock::now();
            func(); 
            auto end = std::chrono::high_resolution_clock::now();

            duration = std::chrono::duration_cast<Duration>(end - start);
        }
    };
}