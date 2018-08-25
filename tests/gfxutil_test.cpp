/*
Copyright (C) 2016 Team Vectrexia

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
#include <gtest/gtest.h>
#include "gfxutil.h"

TEST(Mono, TestARGBChannels)
{
  auto pixel = vxgfx::pf_argb_t(0x7f, 0x7f, 0x7f);

  EXPECT_EQ(static_cast<uint8_t>(pixel.a() * 0xff), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(pixel.r() * 0xff), 0x7f);
  EXPECT_EQ(static_cast<uint8_t>(pixel.g() * 0xff), 0x7f);
  EXPECT_EQ(static_cast<uint8_t>(pixel.b() * 0xff), 0x7f);
}

TEST(Mono, TestRGB565Channels)
{
  // Round trip GREEN
  auto pixel = vxgfx::pf_rgb565_t(0x00, 0xff, 0x00);

  EXPECT_EQ(static_cast<uint8_t>(pixel.r() * 0xff), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(pixel.g() * 0xff), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(pixel.b() * 0xff), 0x00);
}

TEST(ARGB, TestExtractChannelARGB)
{
  auto argb_pixel = vxgfx::pf_argb_t(0x00, 0x10, 0x20, 0x30);

  EXPECT_EQ(argb_pixel.comp_a(argb_pixel), 0x00);
  EXPECT_EQ(argb_pixel.comp_r(argb_pixel), 0x10);
  EXPECT_EQ(argb_pixel.comp_g(argb_pixel), 0x20);
  EXPECT_EQ(argb_pixel.comp_b(argb_pixel), 0x30);
}

TEST(GFXUtil, RectIntersect)
{
    auto a = vxgfx::rect_t(10, 10);
    auto b = vxgfx::rect_t(-2, -2, 14, 14);
    auto c = vxgfx::rect_t(-1, -1, 6,  6);
    auto d = vxgfx::rect_t( 5, -1, 6,  6);
    auto e = vxgfx::rect_t( 5,  5, 6,  6);
    auto f = vxgfx::rect_t(-1,  5, 6,  6);

    EXPECT_EQ(vxgfx::intersect(a, b), vxgfx::rect_t(0, 2, 10, 8));
    EXPECT_EQ(vxgfx::intersect(a, c), vxgfx::rect_t(0, 0, 5, 5));
    EXPECT_EQ(vxgfx::intersect(a, d), vxgfx::rect_t(5, 0, 5, 5));
    EXPECT_EQ(vxgfx::intersect(a, e), vxgfx::rect_t(5, 5, 5, 5));
    EXPECT_EQ(vxgfx::intersect(a, f), vxgfx::rect_t(0, 5, 5, 5));
}

TEST(GFXUtil, RectArea)
{
    auto a = vxgfx::rect_t(10, 10);

    EXPECT_EQ(a.area(), 100);
}

TEST(GFXUtil, RectBoolean)
{
    auto a = vxgfx::rect_t(10, 10);
    auto b = vxgfx::rect_t(0, 0);

    EXPECT_TRUE(a);
    EXPECT_FALSE(b);
}

TEST(GFXUtil, RectMove)
{
    auto a = vxgfx::rect_t(10, 10);
    a.move(10, 10);

    EXPECT_EQ(a, vxgfx::rect_t(10, 10, 10, 10));
}

TEST(GFXUtil, RectOffset)
{
    auto a = vxgfx::rect_t(10, 10, 10, 10);
    a.offset(-10, -10);

    EXPECT_EQ(a, vxgfx::rect_t(0, 0, 10, 10));
}

TEST(GFXUtil, RectInit)
{
    auto a = vxgfx::rect_t();
    auto b = vxgfx::rect_t(10,10);  
    auto c = vxgfx::rect_t(10,10,10,10);
    
    EXPECT_TRUE(a.left == 0 && a.top == 0 && a.right == 0 && a.bottom == 0);
    EXPECT_TRUE(b.left == 0 && b.top == 0 && b.right == 10 && b.bottom == 10);
    EXPECT_TRUE(c.left == 10 && c.top == 10 && c.right == 20 && c.bottom == 20);
}
