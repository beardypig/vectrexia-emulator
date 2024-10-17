/*
Copyright (C) 2016-2024 Team Vectrexia

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

#include <catch2/catch_all.hpp>
#include "gfxutil.h"

TEST_CASE("Mono TestARGBChannels", "[gfxutil]") {
    auto pixel = vxgfx::pf_argb_t(0x7f, 0x7f, 0x7f);

    REQUIRE(static_cast<uint8_t>(pixel.a() * 0xff) == 0xff);
    REQUIRE(static_cast<uint8_t>(pixel.r() * 0xff) == 0x7f);
    REQUIRE(static_cast<uint8_t>(pixel.g() * 0xff) == 0x7f);
    REQUIRE(static_cast<uint8_t>(pixel.b() * 0xff) == 0x7f);
}

TEST_CASE("Mono TestRGB565Channels", "[gfxutil]") {
    // Round trip GREEN
    auto pixel = vxgfx::pf_rgb565_t(0x00, 0xff, 0x00);

    REQUIRE(static_cast<uint8_t>(pixel.r() * 0xff) == 0x00);
    REQUIRE(static_cast<uint8_t>(pixel.g() * 0xff) == 0xff);
    REQUIRE(static_cast<uint8_t>(pixel.b() * 0xff) == 0x00);
}

TEST_CASE("ARGB TestExtractChannelARGB", "[gfxutil]") {
    auto argb_pixel = vxgfx::pf_argb_t(0x00, 0x10, 0x20, 0x30);

    REQUIRE(argb_pixel.comp_a(argb_pixel) == 0x00);
    REQUIRE(argb_pixel.comp_r(argb_pixel) == 0x10);
    REQUIRE(argb_pixel.comp_g(argb_pixel) == 0x20);
    REQUIRE(argb_pixel.comp_b(argb_pixel) == 0x30);
}

TEST_CASE("GFXUtil RectIntersect", "[gfxutil]") {
    auto a = vxgfx::rect_t(10, 10);
    auto b = vxgfx::rect_t(-2, -2, 14, 14);
    auto c = vxgfx::rect_t(-1, -1, 6, 6);
    auto d = vxgfx::rect_t(5, -1, 6, 6);
    auto e = vxgfx::rect_t(5, 5, 6, 6);
    auto f = vxgfx::rect_t(-1, 5, 6, 6);

    REQUIRE(vxgfx::intersect(a, b) == vxgfx::rect_t(0, 2, 10, 8));
    REQUIRE(vxgfx::intersect(a, c) == vxgfx::rect_t(0, 0, 5, 5));
    REQUIRE(vxgfx::intersect(a, d) == vxgfx::rect_t(5, 0, 5, 5));
    REQUIRE(vxgfx::intersect(a, e) == vxgfx::rect_t(5, 5, 5, 5));
    REQUIRE(vxgfx::intersect(a, f) == vxgfx::rect_t(0, 5, 5, 5));
}

TEST_CASE("GFXUtil RectArea", "[gfxutil]") {
    auto a = vxgfx::rect_t(10, 10);
    REQUIRE(a.area() == 100);
}

TEST_CASE("GFXUtil RectBoolean", "[gfxutil]") {
    auto a = vxgfx::rect_t(10, 10);
    auto b = vxgfx::rect_t(0, 0);

    REQUIRE(static_cast<bool>(a));
    REQUIRE_FALSE(static_cast<bool>(b));
}

TEST_CASE("GFXUtil RectMove", "[gfxutil]") {
    auto a = vxgfx::rect_t(10, 10);
    a.move(10, 10);

    REQUIRE(a == vxgfx::rect_t(10, 10, 10, 10));
}

TEST_CASE("GFXUtil RectOffset", "[gfxutil]") {
    auto a = vxgfx::rect_t(10, 10, 10, 10);
    a.offset(-10, -10);

    REQUIRE(a == vxgfx::rect_t(0, 0, 10, 10));
}

TEST_CASE("GFXUtil RectInit", "[gfxutil]") {
    auto a = vxgfx::rect_t();
    auto b = vxgfx::rect_t(10, 10);
    auto c = vxgfx::rect_t(10, 10, 10, 10);

    REQUIRE(a.left == 0);
    REQUIRE(a.top == 0);
    REQUIRE(a.right == 0);
    REQUIRE(a.bottom == 0);

    REQUIRE(b.left == 0);
    REQUIRE(b.top == 0);
    REQUIRE(b.right == 10);
    REQUIRE(b.bottom == 10);

    REQUIRE(c.left == 10);
    REQUIRE(c.top == 10);
    REQUIRE(c.right == 20);
    REQUIRE(c.bottom == 20);
}
