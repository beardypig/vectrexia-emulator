/*
Copyright (C) 2016 beardypig, pelorat

This file is part of Vectrexia.

Vectrexia is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Vectrexia is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Vectrexia.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VECTREXIA_VECLIB_H
#define VECTREXIA_VECLIB_H

#include <string>
#include <cstdio>
#include <cstdarg>
#include <cassert>
#include <deque>

/*
 * Name space for vectrexia utility functions
 */
namespace vxl
{

/*
 * Implementation of std::clamp
 * See https://en.cppreference.com/w/cpp/algorithm/clamp
 * std::clamp is new in C++17
 */
template<class T, class Compare>
constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp) {
    return assert(!comp(hi, lo)),
        comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return clamp(v, lo, hi, std::less<>());
}


/*
 * Simple string formatting function.
 */
inline std::string format(const char *fmt, ...)
{
    va_list ap, ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    std::string out;

    int size = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);
    if (size > 0) {
        out = std::string(size, 0);
        vsnprintf(&out[0], out.size()+1, fmt, ap2);
    }

    va_end(ap2);
    return out;
}

/*
 * Convert CPU cycles to nano seconds.
 */
constexpr uint64_t cycles_to_nanos(uint64_t cycles){
    return (uint64_t)(cycles * (1 / 1.5e-3));
}

/*
 * Convert nano seconds to CPU cycles.
 */
constexpr uint64_t nanos_to_cycles(uint64_t nanos){
    return (uint64_t)(nanos / (1 / 1.5e-3));
}

/**
 * Signal delay deque. Simple wrapper around std::deque
 */
template<typename T>
class delay {
public:
    // The signal output (read after step)
    T output;

    // Constructor; fills the queue with a zero signal
    delay(int cycles, T Vin)
    {
        for (int i = 0; i < cycles; i++)
        {
            queue.emplace_front(Vin);
        }
    }

    void push(T Vin)
    {
        queue.emplace_front(Vin);
    }

    // Pop the signal in the back and push a new one in the front
    void step(T Vin)
    {
        queue.emplace_front(Vin);
        output = queue.back();
        queue.pop_back();
    }

    void step() {
        output = queue.back();
        queue.pop_back();
    }

private:
    std::deque<T> queue;
};

}


#endif // VECTREXIA_VECLIB_H