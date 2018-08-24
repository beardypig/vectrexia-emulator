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

TEST(Mono, TestMonoToARGB)
{
  auto mono_pixel = vxgfx::pf_mono_t(0.5f);
  auto argb_pixel = vxgfx::pf_argb_t(static_cast<uint8_t>(vxl::clamp(mono_pixel.value, 0.0f, 1.0f) * 0xff),
                                     static_cast<uint8_t>(vxl::clamp(mono_pixel.value, 0.0f, 1.0f) * 0xff),
                                     static_cast<uint8_t>(vxl::clamp(mono_pixel.value, 0.0f, 1.0f) * 0xff));

  EXPECT_EQ(static_cast<uint8_t>(argb_pixel.r() * 0xff), 0x7f);
  EXPECT_EQ(static_cast<uint8_t>(argb_pixel.g() * 0xff), 0x7f);
  EXPECT_EQ(static_cast<uint8_t>(argb_pixel.b() * 0xff), 0x7f);
  EXPECT_EQ(static_cast<uint8_t>(argb_pixel.a() * 0xff), 0xff);
}

TEST(ARGB, TestExtractChannel)
{
  auto argb_pixel = vxgfx::pf_argb_t(0x00, 0x10, 0x20, 0x30);

  EXPECT_EQ(argb_pixel.comp_a(argb_pixel), 0x00);
  EXPECT_EQ(argb_pixel.comp_r(argb_pixel), 0x10);
  EXPECT_EQ(argb_pixel.comp_g(argb_pixel), 0x20);
  EXPECT_EQ(argb_pixel.comp_b(argb_pixel), 0x30);
}