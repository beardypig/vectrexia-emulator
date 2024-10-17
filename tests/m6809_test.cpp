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

#include <m6809.h>
#include <catch2/catch_all.hpp> 

auto getRegisters() {
    static M6809 cpu;
    return cpu.getRegisters();
}

TEST_CASE("ComputeFlags ZeroFlagNotSetOn1", "[flags]") {
    auto regs = getRegisters();
    regs.UpdateFlagZero<uint8_t>(1);
    REQUIRE(regs.CC == 0);
}

TEST_CASE("ComputeFlags ZeroFlagSetOn0", "[flags]") {
    auto regs = getRegisters();
    regs.UpdateFlagZero<uint8_t>(0);
    REQUIRE(regs.CC == FLAG_Z);
}

TEST_CASE("ComputeFlags NegativeFlagNotSetOn1", "[flags]") {
    auto regs = getRegisters();
    regs.UpdateFlagNegative<uint8_t>(0);
    REQUIRE(regs.CC == 0);
}

TEST_CASE("ComputeFlags NegativeFlagSetOnFF", "[flags]") {
    auto regs = getRegisters();
    regs.UpdateFlagNegative<uint8_t>(0xff);
    REQUIRE(regs.CC == FLAG_N);
}

TEST_CASE("Misc SignExtend", "[misc]") {
    uint8_t data = 0x10;
    uint16_t sdata = (uint16_t)((~(data & 0x80) + 1) | (data & 0xff));

    REQUIRE(sdata == (int16_t)data);
}
