/*
Copyright (C) 2016-2024 Team Vectrexia

This file is part of Vectrexia.

Vectrexia is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Vectrexia is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Vectrexia. If not, see <http://www.gnu.org/licenses/>.
*/

#include <m6809.h>
#include <stdint.h>

#include <catch2/catch_all.hpp> 
#include <catch2/trompeloeil.hpp> /* this should go last */

using trompeloeil::_;

struct MockMemory
{
   MAKE_MOCK1(Read, auto(uint16_t addr) -> uint8_t);
   MAKE_MOCK2(Write, auto(uint16_t addr, uint8_t value) -> void);
};

static uint8_t read_mem(intptr_t ref, uint16_t addr) {
    return reinterpret_cast<MockMemory*>(ref)->Read(addr);
}

static void write_mem(intptr_t ref, uint16_t addr, uint8_t data) {
    reinterpret_cast<MockMemory*>(ref)->Write(addr, data);
}


M6809 OpCodeTestHelper(MockMemory& mem, trompeloeil::sequence &seq) {
    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x00)
        .IN_SEQUENCE(seq);
    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x00)
        .IN_SEQUENCE(seq);

    M6809 cpu;
    cpu.SetReadCallback(&read_mem, reinterpret_cast<intptr_t>(&mem));
    cpu.SetWriteCallback(&write_mem, reinterpret_cast<intptr_t>(&mem));

    cpu.Reset();

    return cpu;
}
/*
 * ABX
 */

TEST_CASE("M6809OpCodes ABXInherent", "[m6809]") {
    MockMemory mem;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();

    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x3a)
		.IN_SEQUENCE(seq);

    uint64_t cycles = 0;

    registers.B = 0x10;

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.X == 0x10);
}

/*
 * ADDA
 */
TEST_CASE("M6809OpCodes ADDAImmediate", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();


    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x8B)
        .IN_SEQUENCE(seq);
    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x10)
        .IN_SEQUENCE(seq);

    registers.A = 0x10;

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.A == 0x20);
}

TEST_CASE("M6809OpCodes ADDADirect", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();


    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x9B)  // Direct ADDA
        .IN_SEQUENCE(seq);
    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x10)  // Page offset
        .IN_SEQUENCE(seq);
    REQUIRE_CALL(mem, Read(0x0010))
        .RETURN(0x15)
        .IN_SEQUENCE(seq);
    registers.A = 0x10;

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.A == 0x25);
}

TEST_CASE("M6809OpCodes ADDAExtended", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();

    REQUIRE_CALL(mem, Read(0))
        .RETURN(0xBB); // Extended ADDA 
    REQUIRE_CALL(mem, Read(1))
        .RETURN(0x10);
    REQUIRE_CALL(mem, Read(2))
        .RETURN(0x1);
    REQUIRE_CALL(mem, Read(0x1001))
        .RETURN(0x15);

    registers.A = 0x10;

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.A == 0x25);
}

/*
 * ADDD
 */
TEST_CASE("M6809OpCodes ADDDImmediate", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();

    REQUIRE_CALL(mem, Read(0))
        .RETURN(0xC3); // Immediate ADDD
    REQUIRE_CALL(mem, Read(1))
        .RETURN(0x10);
    REQUIRE_CALL(mem, Read(2))
        .RETURN(0x20);

    registers.D = 0x2010;

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.PC == 3);
    REQUIRE(registers.D == 0x3030);
}

/*
 * SUBA
 */
TEST_CASE("M6809OpCodes SUBAImmediate", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();

    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x80)
        .IN_SEQUENCE(seq);
    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x10)
        .IN_SEQUENCE(seq);

    registers.A = 0x10;

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.A == 0x00);
}

TEST_CASE("M6809OpCodes SUBAImmediate2", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();

    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x80)
        .IN_SEQUENCE(seq);
    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x12)
        .IN_SEQUENCE(seq);

    registers.A = 0x10;

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.A == 0xFE);
}

/*
 * BIT - bitwise and, the register should not be updated
 */
TEST_CASE("M6809OpCodes BITAImmediate", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();

    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x85)
        .IN_SEQUENCE(seq);
    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x12)
        .IN_SEQUENCE(seq);

    registers.A = 0xFF;

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.A == 0xFF);
    REQUIRE((registers.CC & (FLAG_Z | FLAG_N | FLAG_V)) == 0);
}

/*
 * EXG - exchange two registers
 */
TEST_CASE("M6809OpCodes EXGRegistersXY", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();

    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x1E)
        .IN_SEQUENCE(seq);
    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x12)
        .IN_SEQUENCE(seq); // X <-> Y

    registers.X = 0x00FF;
    registers.Y = 0xFF00;

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.Y == 0x00FF);
    REQUIRE(registers.X == 0xFF00);
    REQUIRE((registers.CC & (FLAG_Z | FLAG_N | FLAG_V)) == 0);
}

/*
 * TFR - move one register to another
 */
TEST_CASE("M6809OpCodes TFRRegistersXY", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();

    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x1F)
        .IN_SEQUENCE(seq);
    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x12)
        .IN_SEQUENCE(seq); // X -> Y

    registers.X = 0x1111;
    registers.Y = 0x0000;

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.X == 0x1111);
    REQUIRE(registers.Y == 0x1111);
    REQUIRE((registers.CC & (FLAG_Z | FLAG_N | FLAG_V)) == 0);
}

/*
 * BSR - Branch Subroutine
 */
TEST_CASE("M6809OpCodes BSRCorrectJump", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();

    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x8D)
        .IN_SEQUENCE(seq);  // BSR
    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x10)
        .IN_SEQUENCE(seq);

    REQUIRE_CALL(mem, Write(0xffff, 0x02));
    REQUIRE_CALL(mem, Write(0xfffe, 0x00));

    REQUIRE(cpu.Execute(cycles) == E_SUCCESS);
    REQUIRE(registers.PC == 0x0012);
}

/*
 * Illegal Opcode
 */
TEST_CASE("M6809OpCodes IllegalOp", "[m6809]") {
    MockMemory mem;
    uint64_t cycles = 0;
    trompeloeil::sequence seq;
    M6809 cpu = OpCodeTestHelper(mem, seq);
    auto& registers = cpu.getRegisters();

    REQUIRE_CALL(mem, Read(_))
        .RETURN(0x05);

    REQUIRE(cpu.Execute(cycles) == E_UNKNOWN_OPCODE);
}
