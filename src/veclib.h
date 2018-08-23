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

}


#endif // VECTREXIA_VECLIB_H