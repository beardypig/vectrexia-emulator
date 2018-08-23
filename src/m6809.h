/*
Copyright (C) 2016 beardypig

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
#ifndef VECTREXIA_M6809_H
#define VECTREXIA_M6809_H

#include <cstdint>
#include <functional>
#include <array>
#include <iostream>
#include <type_traits>
#include <memory>
#include <ostream>
#include "m6809_disassemble.h"

enum m6809_error_t {
    E_SUCCESS = 0,
    E_UNKNOWN_OPCODE = -1,
    E_UNKNOWN_OPCODE_PAGE1 = -2,
    E_UNKNOWN_OPCODE_PAGE2 = -3,
    E_ILLEGAL_INDEXING_MODE = -4
};

enum flag_mask_t {
    FLAG_M = 0x100,  // MUL flag
    FLAG_E = 0x80,   // Entire flag
    FLAG_F = 0x40,   // Fast interrupt mask
    FLAG_H = 0x20,   // Half carry
    FLAG_I = 0x10,   // Interrupt mask
    FLAG_N = 0x08,   // Negative
    FLAG_Z = 0x04,   // Zero
    FLAG_V = 0x02,   // Overflow
    FLAG_C = 0x01,    // Carry
    FLAGS_MATH = FLAG_H | FLAG_N | FLAG_Z | FLAG_V | FLAG_C
};

enum register_task_t {
    REG_CC = 0x01,
    REG_A  = 0x02,
    REG_B  = 0x04,
    REG_DP = 0x08,
    REG_X  = 0x10,
    REG_Y  = 0x20,
    REG_SP = 0x40, // or REG_USP
    REG_PC = 0x80
};

enum m6809_interrupt_state_t
{
    IRQ_NORMAL,
    IRQ_WAIT,
    IRQ_SYNC
};

enum m6809_interrupt_t
{
    NONE, IRQ, FIRQ, NMI
};

class M6809
{
    using ptr_t = M6809*;

    using read_callback_t = uint8_t (*)(intptr_t, uint16_t);
    using write_callback_t = void (*)(intptr_t, uint16_t, uint8_t);
    using opcode_handler_t = void (*)(M6809 &, uint64_t &);

    const uint16_t RESET_VECTOR = 0xfffe;
    const uint16_t NMI_VECTOR   = 0xfffc;
    const uint16_t IRQ_VECTOR   = 0xfff8;
    const uint16_t FIRQ_VECTOR  = 0xfff6;

    static const uint16_t SWI1_VECTOR  = 0xfffa;
    static const uint16_t SWI2_VECTOR  = 0xfff4;
    static const uint16_t SWI3_VECTOR  = 0xfff2;

    M6809Disassemble dis_;

    struct Registers
    {

        Registers() noexcept {
            InitEXGTable();
        }
        Registers(const Registers &rhs) {
            *this = rhs;
        }
        Registers &operator=(const Registers &rhs) noexcept {
            D = rhs.D;
            X = rhs.X;
            Y = rhs.Y;
            PC = rhs.PC;
            USP = rhs.USP;
            SP = rhs.SP;
            DP = rhs.DP;
            CC = rhs.CC;
            InitEXGTable();
            return *this;
        }
        ~Registers() = default;
        union
        {
            struct
            {
                // Swap for endianness
#ifdef __MSB_FIRST
                uint8_t A, B;
#else
                uint8_t B, A;
#endif
            };
            uint16_t D = 0;
        };
        uint16_t X = 0;
        uint16_t Y = 0;
        uint16_t PC = 0;        // Program Counter
        uint16_t USP = 0;       // User Stack Pointer
        uint16_t SP = 0;        // Stack Pointer
        uint8_t DP = 0;         // Direct Page register
        union
        {
            uint8_t CC = 0;     // Condition Code
            struct
            {
                uint8_t C : 1;  // 01
                uint8_t V : 1;  // 02
                uint8_t Z : 1;  // 04
                uint8_t N : 1;  // 08
                uint8_t I : 1;  // 10
                uint8_t H : 1;  // 20
                uint8_t F : 1;  // 40
                uint8_t E : 1;  // 80
            } flags;
        };

        std::array<uint16_t*, 6> exg_table_16{};
        std::array<uint8_t*, 4> exg_table_8{};
        std::array<uint16_t*, 4> index_mode_register_table{};

        void InitEXGTable() {
            exg_table_16 = { &D, &X, &Y, &USP, &SP, &PC };
            exg_table_8 = {&A, &B, &CC, &DP};
            index_mode_register_table = { &X, &Y, &USP, &SP };
        }

        // update the zero flag, based on value
        template<typename T>
        inline void UpdateFlagZero(const T &value)
        {
            CC &= ~FLAG_Z;
            CC |= FLAG_Z * ((value == 0) ? 1u : 0u);
        };

        // update the negative flag, based on value
        template <typename T>
        inline void UpdateFlagNegative(const T &value)
        {
            CC &= ~FLAG_N;
            CC |= FLAG_N * ((value >> ((sizeof(T) * 8) - 1u)) & 1u);
        };

        // update the carry flag, based on result, the operands, and whether it was a subtraction or addition
        template <typename T, int subtract=0>
        inline void UpdateFlagCarry(const T &opa, const T &opb, const T &result)
        {
            T opb_ = subtract ? ~opb : opb;
            uint16_t flag  = (opa | opb_) & ~result;  // one of the inputs is 1 and output is 0
            flag |= (opa & opb_);                     // both inputs are 1
            CC &= ~FLAG_C;
            flag >>= (sizeof(T) * 8) - 1u;
            // carry flag is the oposite for subtractions
            CC |= FLAG_C * ((flag & 1) ^ subtract);
        };

        // update the half-carry flag, based on the operands and the result
        inline void UpdateFlagHalfCarry(const uint8_t &opa, const uint8_t &opb, const uint8_t &result)
        {
            uint16_t flag  = (opa | opb) & ~result;  // one of the inputs is 1 and output is 0
            flag |= (opa & opb);                     // both inputs are 1
            CC &= ~FLAG_H;
            CC |= FLAG_H * ((flag >> 3) & 1u);
        };

        // update the overflow flag, based on the operands and the result
        template <typename T>
        inline void UpdateFlagOverflow(const T &opa, const T &opb, const T &result)
        {
            // if the sign bit is the same in both operands but
            // different in the result, then there has been an overflow
            auto bits = sizeof(T) * 8;
            auto set = ((opa >> (bits - 1u)) == (opb >> (bits - 1u)) && (opb >> (bits - 1u)) != (result >> (bits - 1u))) ? 1u : 0u;
            CC &= ~FLAG_V;
            CC |= FLAG_V * set;
        }
    } registers;

    // memory accessors
    //   callbacks
    read_callback_t read_callback_func;
    intptr_t read_callback_ref;
    write_callback_t write_callback_func;
    intptr_t write_callback_ref;

    m6809_interrupt_state_t irq_state = IRQ_NORMAL;

    inline uint8_t Read8(const uint16_t &addr)
    {
        return read_callback_func(read_callback_ref, addr);
    }

    inline uint16_t Read16(const uint16_t &addr)
    {
        return (uint16_t) Read8((uint16_t) (addr + 1)) | (uint16_t) Read8((uint16_t) (addr)) << 8;
    }

    inline void Write8(const uint16_t &addr, const uint8_t &data)
    {
        write_callback_func(write_callback_ref, addr, data);
    }

    inline void Write16(const uint16_t &addr, const uint16_t &data)
    {
        Write8(addr, (uint8_t) (data >> 8));
        Write8((const uint16_t &) (addr + 1), (uint8_t) (data & 0xff));
    }

    inline void Push8(uint16_t &sp, const uint8_t &data)
    {
        Write8(--sp, data);
    }

    inline void Push16(uint16_t &sp, const uint16_t &data)
    {
        Push8(sp, (const uint8_t &) (data));
        Push8(sp, (const uint8_t &) (data >> 8));
    }

    inline uint8_t Pull8(uint16_t &sp)
    {
        return Read8(sp++);
    }

    inline uint16_t Pull16(uint16_t &sp)
    {
        return Pull8(sp) << 8 | Pull8(sp);
    }

    // read 8/16 bits from relative to the pc
    inline uint8_t NextOpcode()
    {
        return ReadPC8();
    }

    inline uint8_t ReadPC8()
    {
        return Read8(registers.PC++);
    }

    inline uint16_t ReadPC16()
    {
        uint16_t bytes = Read16(registers.PC);
        registers.PC += 2;
        return bytes;
    }

    /*
     * OpCode Templates
     */
    struct reg_a { uint8_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.A; } };
    struct reg_b { uint8_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.B; } };
    struct reg_d { uint16_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.D; } };
    struct reg_x { uint16_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.X; } };
    struct reg_y { uint16_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.Y; } };
    struct reg_cc { uint8_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.CC; } };
    struct reg_pc { uint16_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.PC; } };
    struct reg_sp { uint16_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.SP; } };
    struct reg_usp { uint16_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.USP; } };

    /*
     * Memory addressing implementations
     */
    struct immediate { };
    struct direct { uint16_t operator()(M6809& cpu, uint64_t &cycles) { return (cpu.registers.DP << 8) | cpu.ReadPC8(); } };
    template<typename T>
    struct relative { uint16_t operator()(M6809& cpu, uint64_t &cycles) {
        return cpu.registers.PC + static_cast<T>((sizeof(T) == 1) ? cpu.ReadPC8() : cpu.ReadPC16());
        } };
    struct extended { uint16_t operator()(M6809& cpu, uint64_t &cycles) { return cpu.ReadPC16(); } };
    struct indexed { uint16_t operator()(M6809& cpu, uint64_t &cycles) {
            uint16_t ea;
            uint8_t post_byte = cpu.ReadPC8();

            uint16_t &reg = *cpu.registers.index_mode_register_table[(post_byte >> 5) & 0x03];  // bits 5+

            //printf("indexed post byte: %02x\n", post_byte);

            if (!(post_byte & 0x80))
            {
                // (+/- 4 bit offset),R
                cycles += 1;
                return (uint16_t) (reg + (int8_t)((post_byte & 0xf) - (post_byte & 0x10)));

            }
            else
            {
                switch (post_byte & 0xf)
                {
                    case 1:
                        // ,R++
                        cycles += 1;
                    case 0:
                        // ,R+
                        cycles += 2;
                        ea = reg;
                        // register is incremented by 1 or 2
                        reg += 1 + (post_byte & 1);
                        break;
                    case 3:
                        // ,--R 
                        cycles += 1;
                    case 2:
                        // ,-R
                        cycles += 2;
                        reg -= (1 + (post_byte & 1));
                        ea = reg;
                        // register is decremented by 1 or 2
                        break;
                    case 4:
                        // ,R
                        ea = reg;
                        break;
                    case 5:
                        // (+/- B), R
                        cycles += 1;
                        ea = reg + (int8_t)cpu.registers.B;
                        break;
                    case 6:
                        // (+/- A), R
                        cycles += 1;
                        ea = reg + (int8_t)cpu.registers.A;
                        break;
                    case 8:
                        // (+/- 7 bit offset), R
                        cycles += 1;
                        ea = reg + (int8_t)cpu.ReadPC8();
                        break;
                    case 9:
                        // (+/- 15 bit offset), R
                        cycles += 4;
                        ea = reg + (int16_t)cpu.ReadPC16();
                        break;
                    case 0xb:
                        // (+/- D), R
                        cycles += 4;
                        ea = reg + (int16_t)cpu.registers.D;
                        break;
                    case 0xc:
                        // (+/- 7 bit offset), PC
                        cycles += 5;
                        ea = reg + (int8_t)cpu.ReadPC8();
                        break;
                    case 0xd:
                        // (+/- 15 bit offset), PC
                        cycles += 5;
                        ea = reg + (int16_t)cpu.ReadPC16();
                        break;
                    case 0xf:
                        cycles += 2;
                        ea = cpu.ReadPC16();
                        break;
                    default:
                        // Illegal
                        return 0;
                }
                // indirect mode
                if ((post_byte >> 4) & 1)
                {
                    cycles += 3;
                    return cpu.Read16(ea);
                }
                else
                {
                    return ea;
                }
            }
        }
    };
    struct inherent { };

    /*
     * Operand Classes
     *
     * RW = 0  ; Read Only
     * RW = 1  ; Read and Write
     * RW = -1 ; Write Only
     */
    template <typename T, typename Fn, int RW=1>
    struct MemoryOperand
    {
        inline T operator ()(M6809 &cpu, uint64_t &cycles, uint16_t &addr) {
            addr = Fn()(cpu, cycles);
            if (RW >= 0)
                return (sizeof(T) == 1) ? cpu.Read8(addr) : cpu.Read16(addr);
            else
                return 0;
        }

        void update(M6809 &cpu, uint64_t &cycles, uint16_t addr, T data) {
            if (RW != 0)
            {
                if (sizeof(T) == 1)
                    cpu.Write8(addr, data);
                else
                    cpu.Write16(addr, data);
            }
        }
    };

    // immediate specialastion
    template <typename T, int RW>
    struct MemoryOperand<T, immediate, RW>
    {
        inline T operator ()(M6809 &cpu, uint64_t &cycles, uint16_t &addr) {
            if (sizeof(T) == 1)
                return cpu.ReadPC8();
            else
                return cpu.ReadPC16();
        }

        inline void update(M6809 &cpu, uint64_t &cycles, uint16_t addr, T data) { }
    };

    template <typename T, typename Fn, int RW=1>
    struct Register
    {
        inline T &operator() (M6809 &cpu, uint64_t &cycles, uint16_t &addr) {
            return Fn()(cpu, addr);
        }

        inline void update(M6809 &cpu, uint64_t &cycles, uint16_t addr, T data) {
            if (RW)
            {
                Fn()(cpu, addr) = data;
            }
        }
    };

    template <typename Fn>
    struct OperandEA 
    {
        inline uint16_t operator ()(M6809 &cpu, uint64_t &cycles, uint16_t &addr) {
            return Fn()(cpu, cycles);
        }

        void update(M6809 &cpu, uint64_t &cycles, uint16_t addr, uint16_t data) {}
    };

    template <typename T, int value>
    struct OperandConst
    {
        inline T operator ()(M6809 &cpu, uint64_t &cycles, uint16_t &addr) {
            return value;
        }

        void update(M6809 &cpu, uint64_t &cycles, uint16_t addr, uint16_t data) {}
    };

    /*
     * Alaises for Registers and memory addressing modes
     */
    using RegisterA   = Register<uint8_t,  reg_a>;
    using RegisterB   = Register<uint8_t,  reg_b>;
    using RegisterD   = Register<uint16_t, reg_d>;
    using RegisterX   = Register<uint16_t, reg_x>;
    using RegisterY   = Register<uint16_t, reg_y>;
    using RegisterSP  = Register<uint16_t, reg_sp>;
    using RegisterUSP = Register<uint16_t, reg_usp>;
    using RegisterCC  = Register<uint8_t,  reg_cc>;
    using RegisterPC  = Register<uint16_t, reg_pc>;

    // read-only version
    using RegisterA_RO    = Register<uint8_t,   reg_a,   0>;
    using RegisterB_RO    = Register<uint8_t,   reg_b,   0>;
    using RegisterD_RO    = Register<uint16_t,  reg_d,   0>;
    using RegisterX_RO    = Register<uint16_t,  reg_x,   0>;
    using RegisterY_RO    = Register<uint16_t,  reg_y,   0>;
    using RegisterCC_RO   = Register<uint8_t,   reg_cc,  0>;
    using RegisterSP_RO   = Register<uint16_t,  reg_sp,  0>;
    using RegisterUSP_RO  = Register<uint16_t,  reg_usp, 0>;

    using ImmediateOperand8  = MemoryOperand<uint8_t,  immediate>;
    using ImmediateOperand16 = MemoryOperand<uint16_t, immediate>;
    using DirectOperand8     = MemoryOperand<uint8_t,  direct>;
    using DirectOperand16    = MemoryOperand<uint16_t, direct>;
    using ExtendedOperand8   = MemoryOperand<uint8_t,  extended>;
    using ExtendedOperand16  = MemoryOperand<uint16_t, extended>;
    using IndexedOperand8    = MemoryOperand<uint8_t,  indexed>;
    using IndexedOperand16   = MemoryOperand<uint16_t, indexed>;

    // Update only memory operands
    using DirectOperand8_WO    = MemoryOperand<uint8_t,  direct,   -1>;
    using DirectOperand16_WO   = MemoryOperand<uint16_t, direct,   -1>;
    using ExtendedOperand8_WO  = MemoryOperand<uint8_t,  extended, -1>;
    using ExtendedOperand16_WO = MemoryOperand<uint16_t, extended, -1>;
    using IndexedOperand8_WO   = MemoryOperand<uint8_t,  indexed,  -1>;
    using IndexedOperand16_WO  = MemoryOperand<uint16_t, indexed,  -1>;

    using DirectEA       = OperandEA<direct>;
    using IndexedEA      = OperandEA<indexed>;
    using ExtendedEA     = OperandEA<extended>;
    using RelativeEA     = OperandEA<relative<int8_t>>;
    using RelativeEALong = OperandEA<relative<int16_t>>;

    /*
     * Calculate flags
     */
    template <int FlagUpdateMask=0, int FlagSetMask=0, int FlagClearMask=0, int subtract=0, typename T=uint8_t, typename T2=T>
    struct compute_flags
    {
        inline void operator() (M6809 &cpu, T &result, T &operand_a, T2 &operand_b)
        {
            uint8_t CC = cpu.registers.CC;

            if (FlagClearMask) cpu.registers.CC &= ~FlagClearMask;
            if (FlagSetMask) cpu.registers.CC |= FlagSetMask;

            if (FlagUpdateMask & FLAG_Z) cpu.registers.UpdateFlagZero<T>(result);
            if (FlagUpdateMask & FLAG_N) cpu.registers.UpdateFlagNegative(result);
            if (FlagUpdateMask & FLAG_H)
                cpu.registers.UpdateFlagHalfCarry(operand_a, subtract ? ~operand_b : operand_b, result);
            if (FlagUpdateMask & FLAG_V)
                cpu.registers.UpdateFlagOverflow<T>(operand_a, subtract ? ~operand_b : operand_b, result);
            if (FlagUpdateMask & FLAG_C) cpu.registers.UpdateFlagCarry<T, subtract>(operand_a, operand_b, result);
            if (FlagUpdateMask & FLAG_M)
            {
                cpu.registers.CC &= ~FLAG_C;
                cpu.registers.CC |= FLAG_C * ((result >> 7) & 1);
            }
        }
    };

    // Aliases for common flags
    using FlagMaths = compute_flags<FLAGS_MATH>;
    using FlagMathsSub = compute_flags<FLAGS_MATH, 0, 0, 1>;
    using FlagMaths16 = compute_flags<FLAG_N | FLAG_Z | FLAG_V | FLAG_C, 0, 0, 0, uint16_t>;
    using FlagMaths16Sub = compute_flags<FLAG_N | FLAG_Z | FLAG_V | FLAG_C, 0, 0, 1, uint16_t>;
    using LogicFlags = compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>;
    using NoFlags16 = compute_flags<0,0,0,0,uint16_t>;

    /*
     * Opcode implementations
     */
    template <typename T1, typename T2=T1>
    struct op_add { T1 operator() (M6809& cpu, const T1 &operand_a, const T2 &operand_b) { return operand_a + operand_b; } };

    struct op_adc { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b) { return operand_a + operand_b + cpu.registers.flags.C; } };
    struct op_sbc { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b)
        { return static_cast<uint8_t>( operand_a -  static_cast<int8_t>(operand_b) -  static_cast<int8_t>(cpu.registers.flags.C)); }
    };
    template <typename T>
    struct op_sub {
        T operator() (const M6809& cpu, const T &operand_a, const T &operand_b) {
            return operand_a + ~operand_b + 1;
        }
    };

    struct op_eor { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b) { return operand_a ^ operand_b; } };
    struct op_and { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b) { return operand_a & operand_b; } };
    struct op_or { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b) { return operand_a | operand_b; } };
    struct op_sex { uint16_t operator() (M6809& cpu, const uint16_t &operand_a, const uint8_t &operand_b)
        {
            auto res = (uint16_t) ((~(operand_b & 0x80) + 1) | (operand_b & 0xff));
            // special case for N and Z flags
            cpu.registers.UpdateFlagNegative<uint8_t>((const uint8_t &) (res & 0xff));
            cpu.registers.UpdateFlagZero<uint16_t>(res);
            return res;
        }
    };
    // Push PC on to the SP, and set the PC to the operand
    template <typename T=uint16_t>
    struct op_jsr {
        uint16_t operator() (M6809& cpu, const uint16_t &operand_a, const uint16_t &operand_b)
        {
            // push PC
            cpu.Push16(cpu.registers.SP, cpu.registers.PC);
            return operand_b;
        }
    };

    // store/load, M <= Register or Register <= Memory
    template <typename T>
    struct op_copy { T operator() (const M6809& cpu, const T &operand_a, const T &operand_b) { return operand_b; } };

    // This is a special case where the operation sets a pseudo flag to tell the cpu to wait for an interrupt
    struct op_cwai {
        uint8_t operator() (M6809& cpu, const uint8_t &operand, uint64_t &cycles) {
            cpu.registers.CC &= operand;
            cpu.irq_state = IRQ_WAIT;
            cpu.registers.flags.E = 1;
            // push all the registers
            op_push<reg_sp, reg_usp>()(cpu, 0xff, cycles);
            return 0;
        }
    };


    // one operand
    struct op_daa {
        uint8_t operator() (M6809& cpu, const uint8_t &operand) {
            uint8_t result = operand;
            if (cpu.registers.flags.H || (operand & 0xf) > 9)
            {
                result += 6;
            }
            if (cpu.registers.flags.C || result > 0x9f)
            {
                result += 0x60;
            }
            return result;
        }
    };
    struct op_mul { uint16_t operator() (const M6809& cpu, const uint8_t &operand) { return cpu.registers.A * cpu.registers.B; } };
    struct op_clr { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return 0; } };
    struct op_asr { uint8_t operator() (M6809& cpu, const uint8_t &operand)
        {
            //special case for asr flag
            cpu.registers.CC &= ~FLAG_C;
            cpu.registers.CC |= FLAG_C * (operand & 1);
            return (uint8_t) (((operand >> 1) & 0x7f) | (operand & 0x80));
        }
    };
    struct op_com { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return ~operand; } };
    struct op_lsl {
        uint8_t operator() (M6809& cpu, const uint8_t &operand) {
            // special case for the H,V and C flags for LSL/ASL
            uint8_t res = operand << 1;
            cpu.registers.UpdateFlagCarry<uint8_t>(operand, operand, res);
            cpu.registers.UpdateFlagCarry(operand, operand, res);
            cpu.registers.UpdateFlagOverflow<uint8_t>(operand, operand, res);
            return res;
        }
    };
    struct op_lsr {
        uint8_t operator() (M6809& cpu, const uint8_t &operand) {
            // special case for the C flag for LSR
            cpu.registers.CC &= ~FLAG_C;
            cpu.registers.CC |= FLAG_C * (operand & 1);
            return operand >> 1;
        }
    };
    struct op_neg { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return (uint8_t) (~operand + 1); } };
    struct op_tst { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return operand; } };
    struct op_ror {
        uint8_t operator() (M6809& cpu, const uint8_t &operand)
        {
            auto res = (uint8_t) (((operand >> 1) & 0x7f) | (cpu.registers.flags.C << 7));
            // special case for C flag
            cpu.registers.CC &= ~FLAG_C;
            cpu.registers.CC |= FLAG_C * (operand & 1);
            return res;
        }
    };
    struct op_rol {
        uint8_t operator() (M6809& cpu, const uint8_t &operand)
        {
            uint8_t res = (operand << 1) | cpu.registers.flags.C;
            cpu.registers.UpdateFlagCarry<uint8_t>(operand, operand, res);
            cpu.registers.UpdateFlagOverflow<uint8_t>(operand, operand, res);
            return res;
        }
    };
    struct op_rts { uint16_t operator() (M6809& cpu, const uint8_t &operand) { return cpu.Pull16(cpu.registers.SP); } };
    struct op_rti {
        // pull the registers and then the pc
        uint16_t operator() (M6809& cpu, const uint8_t &operand, uint64_t &cycles) {
            uint8_t register_mask = (cpu.registers.flags.E) ? (uint8_t)0xff : (uint8_t)0x81;
            op_pull<reg_sp, reg_usp>()(cpu, register_mask, cycles);
            return cpu.registers.PC;
        }
    };

    // assign one register to the other and resize
    template <typename T1, typename T2>
    static void op_reg_assign(T1 *reg_1, const T2 *reg_2) {
      *reg_1 = *reg_2;
    }

    template <typename T1 = uint8_t, typename T2 = uint16_t>
    static void op_reg_assign(uint8_t *reg_1, const uint16_t *reg_2) {
      *reg_1 = static_cast<uint8_t>((*reg_2) >> 8);
    }

    template <typename T1 = uint16_t, typename T2 = uint8_t>
    static void op_reg_assign(uint16_t *reg_1, const uint8_t *reg_2) {
      *reg_1 = static_cast<uint16_t>((*reg_2) | 0xFF00);
    }

    // swap two registers
    template <typename T1, typename T2>
    static void op_swap_registers(T1 *reg_1, T2 *reg_2) {
        T2 temp = *reg_2;
        op_reg_assign(reg_2, reg_1);
        op_reg_assign(reg_1, &temp);
    }

    /*
     * EXG
     *
     * Exchange uses a post byte operand that selects the registers to exchange
     * The first 4 bit select the first register, and the second 4 bits select
     * the second register.
     *    0x00 D 16 bit |  0x05 PC 16 bit
     *    0x01 X 16 bit |  0x08 A   8 bit
     *    0x02 Y 16 bit |  0x09 B   8 bit
     *    0x03 U 16 bit |  0x0A CC  8 bit
     *    0x04 S 16 bit |  0x0B DP  8 bit
     */
    struct op_exg {
        uint8_t operator() (M6809& cpu, const uint8_t &operand) {
            const uint8_t reg0 = (operand >> 4) & 0xf;
            const uint8_t reg1 = operand & 0xf;

            if ((reg0 & 0x8) && (reg1 & 0x08)) // both are 8 bit
                op_swap_registers(cpu.registers.exg_table_8[reg1 & 0x7],
                                  cpu.registers.exg_table_8[reg0 & 0x7]);
            else if (!(reg0 & 0x8) && !(reg1 & 0x08))  // both are 16 bit
                op_swap_registers(cpu.registers.exg_table_16[reg1],
                                  cpu.registers.exg_table_16[reg0]);
            else if ((reg0 & 0x8))  // reg 0 is 8 bit and reg 1 is 16 bit
                op_swap_registers(cpu.registers.exg_table_16[reg1],
                                  cpu.registers.exg_table_8[reg0 & 0x7]);
            else if ((reg1 & 0x8))  // reg 0 is 16 bit and reg 1 is 8 bit
                op_swap_registers(cpu.registers.exg_table_8[reg1 & 0x7],
                                  cpu.registers.exg_table_16[reg0]);
            return 0;
        }
    };

    struct op_tfr {
        uint8_t operator() (M6809& cpu, const uint8_t &operand) {
            const auto reg0 = (operand >> 4) & 0xf;
            const auto reg1 = operand & 0xf;

            if ((reg0 & 0x8) && (reg1 & 0x08)) // both are 8 bit
                op_reg_assign(cpu.registers.exg_table_8[reg1 & 0x7],
                              cpu.registers.exg_table_8[reg0 & 0x7]);
            else if ((reg0 & 0x8))  // reg 0 is 8 bit and reg 1 is 16 bit
                op_reg_assign(cpu.registers.exg_table_16[reg1],
                              cpu.registers.exg_table_8[reg0 & 0x7]);
            else if ((reg1 & 0x8))  // reg 0 is 16 bit and reg 1 is 8 bit
                op_reg_assign(cpu.registers.exg_table_8[reg1 & 0x7],
                              cpu.registers.exg_table_16[reg0]);
            else  // both are 16 bit
                op_reg_assign(cpu.registers.exg_table_16[reg1],
                              cpu.registers.exg_table_16[reg0]);
            return 0;
        }
    };

    // push register on to the stack
    template <typename SP, typename Push_SP=SP>
    struct op_push
    {
        uint8_t operator() (M6809& cpu, const uint8_t &operand, uint64_t &cycles)
        {
            // operand contains a bitmask of the registers to push
            uint16_t &sp = SP()(cpu, 0);
            // the stack pointer to push to the stack
            uint16_t &psp = Push_SP()(cpu, 0);

            if (operand & REG_PC)
            {
                cpu.Push16(sp, cpu.registers.PC);
                cycles += 2;
            }
            if (operand & REG_SP)
            {
                cpu.Push16(sp, psp);
                cycles += 2;
            }
            if (operand & REG_Y)
            {
                cpu.Push16(sp, cpu.registers.Y);
                cycles += 2;
            }
            if (operand & REG_X)
            {
                cpu.Push16(sp, cpu.registers.X);
                cycles += 2;
            }
            if (operand & REG_DP)
            {
                cpu.Push8(sp, cpu.registers.DP);
                cycles += 1;
            }
            if (operand & REG_B)
            {
                cpu.Push8(sp, cpu.registers.B);
                cycles += 1;
            }
            if (operand & REG_A)
            {
                cpu.Push8(sp, cpu.registers.A);
                cycles += 1;
            }
            if (operand & REG_CC)
            {
                cpu.Push8(sp, cpu.registers.CC);
                cycles += 1;
            }
            return 0;
        }
    };

    // pull register from the stack
    template <typename SP, typename Pull_SP=SP>
    struct op_pull
    {
        uint8_t operator() (M6809& cpu, uint8_t &operand, uint64_t &cycles)
        {
            // operand contains a bitmask of the registers to push
            uint16_t &sp = SP()(cpu, 0);

            // the stack pointer to pull from the stack
            uint16_t &psp = Pull_SP()(cpu, 0);
            if (operand & REG_CC)
            {
                cpu.registers.CC = cpu.Pull8(sp);
                cycles += 1;
            }
            if (operand & REG_A)
            {
                cpu.registers.A = cpu.Pull8(sp);
                cycles += 1;
            }
            if (operand & REG_B)
            {
                cpu.registers.B = cpu.Pull8(sp);
                cycles += 1;
            }
            if (operand & REG_DP)
            {
                cpu.registers.DP = cpu.Pull8(sp);
                cycles += 1;
            }
            if (operand & REG_X)
            {
                cpu.registers.X = cpu.Pull16(sp);
                cycles += 2;
            }
            if (operand & REG_Y)
            {
                cpu.registers.Y = cpu.Pull16(sp);
                cycles += 2;
            }
            if (operand & REG_SP)
            {
                psp = cpu.Pull16(sp);
                cycles += 2;
            }
            if (operand & REG_PC)
            {
                cpu.registers.PC = cpu.Pull16(sp);
                cycles += 2;
            }
            return 0;
        }
    };

    template <uint16_t vector, bool set_fi=false>
    struct op_swi
    {
        uint16_t operator()(M6809& cpu, const uint16_t &operand, uint64_t &cycles)
        {
            cpu.registers.CC |= 1 * FLAG_E;
            op_push<reg_sp, reg_usp>()(cpu, 0xff, cycles);  // push all the registers and the usp
            if (set_fi)
                cpu.registers.CC |= (1 * FLAG_F) | (1 * FLAG_I);

            return cpu.Read16(vector);
        }
    };

    // Branch operators

    struct op_bra_always { bool operator ()(M6809 &cpu) { return true; } }; // always
    struct op_bra_carry { bool operator ()(M6809 &cpu) { return cpu.registers.flags.C; } };
    struct op_bra_less { bool operator ()(M6809 &cpu) { return !(cpu.registers.flags.Z | cpu.registers.flags.C); } };
    struct op_bra_equal { bool operator ()(M6809 &cpu) { return cpu.registers.flags.Z; } };
    struct op_bra_less_than { bool operator ()(M6809 &cpu) { return cpu.registers.flags.N ^ cpu.registers.flags.V; } };
    struct op_bra_less_eq
    { bool operator ()(M6809 &cpu) { return cpu.registers.flags.Z | (cpu.registers.flags.N ^ cpu.registers.flags.V); } };
    struct op_bra_plus { bool operator ()(M6809 &cpu) { return !cpu.registers.flags.N; } };
    struct op_bra_overflow { bool operator ()(M6809 &cpu) { return cpu.registers.flags.V; } };

    template <typename Test, typename T, bool Negate=false>
    struct op_bra {
        uint16_t operator()(M6809 &cpu, uint16_t &pc, uint64_t &cycles)
        {
            T offset = (sizeof(T) == 1) ? cpu.ReadPC8() : cpu.ReadPC16();
            auto test = Test()(cpu);
            if (Negate) test = !test;
            if (test)
            {
                cycles += sizeof(T) - 1;  // extra cycle to take the branch
                // cast to correct sized signed int
                return static_cast<uint16_t>(pc + ((sizeof(T) == 1) ? static_cast<int8_t>(offset) : static_cast<int16_t>(offset)) + sizeof(T));
            }
            return static_cast<uint16_t>(pc + sizeof(T));
        }
    };

    template <typename Fn, bool negate=false>
    using op_bra_short = op_bra<Fn, int8_t, negate>;

    template <typename Fn, bool negate=false>
    using op_bra_long = op_bra<Fn, int16_t, negate>;

    // no operands
    struct op_nop { uint8_t operator() (const M6809& cpu) { return 0u; } };
    struct op_reset { uint8_t operator() (M6809& cpu) { cpu.Reset(); return 0; } };
    struct op_sync { uint8_t operator() (M6809& cpu)
        {
            cpu.irq_state = IRQ_SYNC;
            return 0;
        }
    };

    /*
     * Opcode Templates
     */
    template<typename Fn, typename OpA=inherent, typename OpB=inherent, typename Flags=compute_flags<>, int clocks=2>
    struct opcode
    {
        void operator() (M6809& cpu, uint64_t &cycles)
        {
            uint16_t operand_addr_a = 0;
            uint16_t operand_addr_b = 0;

            // gets the value for the operand and updates the address
            auto operand_a_value = OpA()(cpu, cycles, operand_addr_a);
            auto operand_b_value = OpB()(cpu, cycles, operand_addr_b);

            auto result = Fn()(cpu, operand_a_value, operand_b_value);

            Flags()(cpu, result, operand_a_value, operand_b_value);

            OpA().update(cpu, cycles, operand_addr_a, result);
            cycles += clocks;
        }
    };

    template<typename Fn, typename OpA, typename Flags, int clocks>
    struct opcode<Fn, OpA, inherent, Flags, clocks>
    {
        void operator() (M6809& cpu, uint64_t &cycles)
        {
            uint16_t operand_addr = 0;
            auto operand_value = OpA()(cpu, cycles, operand_addr);
            auto result = Fn()(cpu, operand_value);
            decltype(operand_value) zero = 0;

            Flags()(cpu, result, zero, operand_value);

            OpA().update(cpu, cycles, operand_addr, result);
            cycles += clocks;
        }
    };

    template<typename Fn, typename Flags, int clocks>
    struct opcode<Fn, inherent, inherent, Flags, clocks>
    {
        void operator() (M6809& cpu, uint64_t &cycles)
        {
            auto result = Fn()(cpu);
            decltype(result) zero = 0;
            Flags()(cpu, result, zero, zero);
            cycles += clocks;
        }
    };

    // opcode with counting of extra cycles
    template<typename Fn, typename OpA=inherent, typename OpB=inherent, typename Flags=compute_flags<>, int clocks=0>
    struct opcode_count
    {
        void operator() (M6809& cpu, uint64_t &cycles)
        {
            uint16_t operand_addr = 0;
            auto operand_value = OpA()(cpu, cycles, operand_addr);
            auto result = Fn()(cpu, operand_value, cycles);
            decltype(operand_value) zero = 0;

            Flags()(cpu, result, zero, operand_value);

            OpA().update(cpu, cycles, operand_addr, result);
            cycles += clocks;
        }
    };

    template<typename Op>
    static void opcodewrap (M6809& cpu, uint64_t &cycles)
    {
        Op()(cpu, cycles);
    }

    /*
     * Opcode handler definitions
     */
    std::array<opcode_handler_t, 0x100> opcode_handlers;
    std::array<opcode_handler_t, 0x100> opcode_handlers_page1;
    std::array<opcode_handler_t, 0x100> opcode_handlers_page2;

    // ABX
    using op_abx_inherent = opcode<op_add<uint16_t, uint8_t>, RegisterX, RegisterB, compute_flags<0, 0, 0, 0, uint16_t, uint8_t>, 3>;

    // ADC - ADD with carry
    using op_adca_immediate = opcode<op_adc, RegisterA,        ImmediateOperand8, FlagMaths, 2>;
    using op_adca_direct    = opcode<op_adc, RegisterA,        DirectOperand8,    FlagMaths, 4>;
    using op_adca_indexed   = opcode<op_adc, RegisterA,        IndexedOperand8,   FlagMaths, 4>;
    using op_adca_extended  = opcode<op_adc, RegisterA,        ExtendedOperand8,  FlagMaths, 5>;

    using op_adcb_immediate = opcode<op_adc, RegisterB,        ImmediateOperand8, FlagMaths, 2>;
    using op_adcb_direct    = opcode<op_adc, RegisterB,        DirectOperand8,    FlagMaths, 4>;
    using op_adcb_indexed   = opcode<op_adc, RegisterB,        IndexedOperand8,   FlagMaths, 4>;
    using op_adcb_extended  = opcode<op_adc, RegisterB,        ExtendedOperand8,  FlagMaths, 5>;

    // ADD
    using op_adda_immediate = opcode<op_add<uint8_t>,   RegisterA,          ImmediateOperand8,  FlagMaths, 2>;
    using op_adda_direct    = opcode<op_add<uint8_t>,   RegisterA,          DirectOperand8,     FlagMaths, 4>;
    using op_adda_indexed   = opcode<op_add<uint8_t>,   RegisterA,          IndexedOperand8,    FlagMaths, 4>;
    using op_adda_extended  = opcode<op_add<uint8_t>,   RegisterA,          ExtendedOperand8,   FlagMaths, 5>;

    using op_addb_immediate = opcode<op_add<uint8_t>,   RegisterB,          ImmediateOperand8,  FlagMaths, 2>;
    using op_addb_direct    = opcode<op_add<uint8_t>,   RegisterB,          DirectOperand8,     FlagMaths, 4>;
    using op_addb_indexed   = opcode<op_add<uint8_t>,   RegisterB,          IndexedOperand8,    FlagMaths, 4>;
    using op_addb_extended  = opcode<op_add<uint8_t>,   RegisterB,          ExtendedOperand8,   FlagMaths, 5>;

    using op_addd_immediate = opcode<op_add<uint16_t>,  RegisterD,          ImmediateOperand16, FlagMaths16, 4>;
    using op_addd_direct    = opcode<op_add<uint16_t>,  RegisterD,          DirectOperand16,    FlagMaths16, 6>;
    using op_addd_indexed   = opcode<op_add<uint16_t>,  RegisterD,          IndexedOperand16,   FlagMaths16, 6>;
    using op_addd_extended  = opcode<op_add<uint16_t>,  RegisterD,          ExtendedOperand16,  FlagMaths16, 7>;

    // AND
    using op_anda_immediate = opcode<op_and,            RegisterA,          ImmediateOperand8,  LogicFlags, 2>;
    using op_anda_direct    = opcode<op_and,            RegisterA,          DirectOperand8,     LogicFlags, 4>;
    using op_anda_indexed   = opcode<op_and,            RegisterA,          IndexedOperand8,    LogicFlags, 4>;
    using op_anda_extended  = opcode<op_and,            RegisterA,          ExtendedOperand8,   LogicFlags, 5>;

    using op_andb_immediate = opcode<op_and,            RegisterB,          ImmediateOperand8,  LogicFlags, 2>;
    using op_andb_direct    = opcode<op_and,            RegisterB,          DirectOperand8,     LogicFlags, 4>;
    using op_andb_indexed   = opcode<op_and,            RegisterB,          IndexedOperand8,    LogicFlags, 4>;
    using op_andb_extended  = opcode<op_and,            RegisterB,          ExtendedOperand8,   LogicFlags, 5>;

    // ANDCC - do not update the flags, they are affected by the immediate value
    using op_andcc_immediate = opcode<op_and,           RegisterCC,         ImmediateOperand8, compute_flags<>, 3>;

    // ASL (see LSL)
    // ASR
    using op_asra_inherent = opcode<op_asr,             RegisterA,          inherent,           compute_flags<FLAG_N|FLAG_Z>, 2>;
    using op_asrb_inherent = opcode<op_asr,             RegisterB,          inherent,           compute_flags<FLAG_N|FLAG_Z>, 2>;

    using op_asr_direct    = opcode<op_asr,             DirectOperand8,     inherent,           compute_flags<FLAG_N|FLAG_Z>, 6>;
    using op_asr_indexed   = opcode<op_asr,             IndexedOperand8,    inherent,           compute_flags<FLAG_N|FLAG_Z>, 6>;
    using op_asr_extended  = opcode<op_asr,             ExtendedOperand8,   inherent,           compute_flags<FLAG_N|FLAG_Z>, 7>;

    // BIT - bitwise AND, update flags but don't store the result
    using op_bita_immediate = opcode<op_and, RegisterA_RO, ImmediateOperand8, LogicFlags, 2>;
    using op_bita_direct    = opcode<op_and, RegisterA_RO, DirectOperand8,    LogicFlags, 4>;
    using op_bita_indexed   = opcode<op_and, RegisterA_RO, IndexedOperand8,   LogicFlags, 4>;
    using op_bita_extended  = opcode<op_and, RegisterA_RO, ExtendedOperand8,  LogicFlags, 5>;

    using op_bitb_immediate = opcode<op_and, RegisterB_RO, ImmediateOperand8, LogicFlags, 2>;
    using op_bitb_direct    = opcode<op_and, RegisterB_RO, DirectOperand8,    LogicFlags, 4>;
    using op_bitb_indexed   = opcode<op_and, RegisterB_RO, IndexedOperand8,   LogicFlags, 4>;
    using op_bitb_extended  = opcode<op_and, RegisterB_RO, ExtendedOperand8,  LogicFlags, 5>;

    // CLR
    using op_clra_inherent = opcode<op_clr, RegisterA,        inherent, compute_flags<0, FLAG_Z, FLAG_N|FLAG_V|FLAG_C>, 2>;
    using op_clrb_inherent = opcode<op_clr, RegisterB,        inherent, compute_flags<0, FLAG_Z, FLAG_N|FLAG_V|FLAG_C>, 2>;

    // these memory operands are update only
    using op_clr_direct    = opcode<op_clr, DirectOperand8_WO,   inherent, compute_flags<0, FLAG_Z, FLAG_N|FLAG_V|FLAG_C>, 6>;
    using op_clr_indexed   = opcode<op_clr, IndexedOperand8_WO,  inherent, compute_flags<0, FLAG_Z, FLAG_N|FLAG_V|FLAG_C>, 6>;
    using op_clr_extended  = opcode<op_clr, ExtendedOperand8_WO, inherent, compute_flags<0, FLAG_Z, FLAG_N|FLAG_V|FLAG_C>, 7>;

    // CMP
    using op_cmpa_immediate = opcode<op_sub<uint8_t>,   RegisterA_RO,   ImmediateOperand8,  FlagMathsSub, 2>;
    using op_cmpa_direct =    opcode<op_sub<uint8_t>,   RegisterA_RO,   DirectOperand8,     FlagMathsSub, 4>;
    using op_cmpa_indexed =   opcode<op_sub<uint8_t>,   RegisterA_RO,   IndexedOperand8,    FlagMathsSub, 4>;
    using op_cmpa_extended =  opcode<op_sub<uint8_t>,   RegisterA_RO,   ExtendedOperand8,   FlagMathsSub, 5>;

    using op_cmpb_immediate = opcode<op_sub<uint8_t>,   RegisterB_RO,   ImmediateOperand8,  FlagMathsSub, 2>;
    using op_cmpb_direct =    opcode<op_sub<uint8_t>,   RegisterB_RO,   DirectOperand8,     FlagMathsSub, 4>;
    using op_cmpb_indexed =   opcode<op_sub<uint8_t>,   RegisterB_RO,   IndexedOperand8,    FlagMathsSub, 4>;
    using op_cmpb_extended =  opcode<op_sub<uint8_t>,   RegisterB_RO,   ExtendedOperand8,   FlagMathsSub, 5>;

    using op_cmpd_immediate = opcode<op_sub<uint16_t>,  RegisterD_RO,   ImmediateOperand16,  FlagMaths16Sub, 5>;
    using op_cmpd_direct =    opcode<op_sub<uint16_t>,  RegisterD_RO,   DirectOperand16,     FlagMaths16Sub, 7>;
    using op_cmpd_indexed =   opcode<op_sub<uint16_t>,  RegisterD_RO,   IndexedOperand16,    FlagMaths16Sub, 7>;
    using op_cmpd_extended =  opcode<op_sub<uint16_t>,  RegisterD_RO,   ExtendedOperand16,   FlagMaths16Sub, 8>;

    using op_cmps_immediate = opcode<op_sub<uint16_t>,  RegisterSP_RO,  ImmediateOperand16,  FlagMaths16Sub, 5>;
    using op_cmps_direct =    opcode<op_sub<uint16_t>,  RegisterSP_RO,  DirectOperand16,     FlagMaths16Sub, 7>;
    using op_cmps_indexed =   opcode<op_sub<uint16_t>,  RegisterSP_RO,  IndexedOperand16,    FlagMaths16Sub, 7>;
    using op_cmps_extended =  opcode<op_sub<uint16_t>,  RegisterSP_RO,  ExtendedOperand16,   FlagMaths16Sub, 8>;

    using op_cmpu_immediate = opcode<op_sub<uint16_t>,  RegisterUSP_RO, ImmediateOperand16,  FlagMaths16Sub, 5>;
    using op_cmpu_direct =    opcode<op_sub<uint16_t>,  RegisterUSP_RO, DirectOperand16,     FlagMaths16Sub, 7>;
    using op_cmpu_indexed =   opcode<op_sub<uint16_t>,  RegisterUSP_RO, IndexedOperand16,    FlagMaths16Sub, 7>;
    using op_cmpu_extended =  opcode<op_sub<uint16_t>,  RegisterUSP_RO, ExtendedOperand16,   FlagMaths16Sub, 8>;

    using op_cmpx_immediate = opcode<op_sub<uint16_t>,  RegisterX_RO,   ImmediateOperand16,  FlagMaths16Sub, 4>;
    using op_cmpx_direct =    opcode<op_sub<uint16_t>,  RegisterX_RO,   DirectOperand16,     FlagMaths16Sub, 6>;
    using op_cmpx_indexed =   opcode<op_sub<uint16_t>,  RegisterX_RO,   IndexedOperand16,    FlagMaths16Sub, 6>;
    using op_cmpx_extended =  opcode<op_sub<uint16_t>,  RegisterX_RO,   ExtendedOperand16,   FlagMaths16Sub, 7>;

    using op_cmpy_immediate = opcode<op_sub<uint16_t>,  RegisterY_RO,   ImmediateOperand16,  FlagMaths16Sub, 5>;
    using op_cmpy_direct =    opcode<op_sub<uint16_t>,  RegisterY_RO,   DirectOperand16,     FlagMaths16Sub, 7>;
    using op_cmpy_indexed =   opcode<op_sub<uint16_t>,  RegisterY_RO,   IndexedOperand16,    FlagMaths16Sub, 7>;
    using op_cmpy_extended =  opcode<op_sub<uint16_t>,  RegisterY_RO,   ExtendedOperand16,   FlagMaths16Sub, 8>;

    // COM
    using op_coma_inherent = opcode<op_com,  RegisterA,        inherent, compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>, 2>;
    using op_comb_inherent = opcode<op_com,  RegisterB,        inherent, compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>, 2>;

    using op_com_direct     = opcode<op_com, DirectOperand8,   inherent, compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>, 6>;
    using op_com_indexed    = opcode<op_com, IndexedOperand8,  inherent, compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>, 6>;
    using op_com_extended   = opcode<op_com, ExtendedOperand8, inherent, compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>, 7>;

    // CWAI - pushes the registers ... which takes a while
    using op_cwai_immediate = opcode_count<op_cwai, ImmediateOperand8, inherent, compute_flags<>, 8>;

    // DAA
    using op_daa_inherent   = opcode<op_daa,            RegisterA,          inherent,           compute_flags<FLAG_C|FLAG_Z|FLAG_N, 0, FLAG_V>, 2>;

    // DEC
    using op_deca_inherent  = opcode<op_sub<uint8_t>,   RegisterA,          OperandConst<uint8_t, 1>,    compute_flags<FLAG_V|FLAG_Z|FLAG_N, 0, 0, 1>, 2>;
    using op_decb_inherent  = opcode<op_sub<uint8_t>,   RegisterB,          OperandConst<uint8_t, 1>,    compute_flags<FLAG_V|FLAG_Z|FLAG_N, 0, 0, 1>, 2>;

    using op_dec_direct     = opcode<op_sub<uint8_t>,   DirectOperand8,     OperandConst<uint8_t, 1>,    compute_flags<FLAG_V|FLAG_Z|FLAG_N, 0, 0, 1>, 6>;
    using op_dec_indexed    = opcode<op_sub<uint8_t>,   IndexedOperand8,    OperandConst<uint8_t, 1>,    compute_flags<FLAG_V|FLAG_Z|FLAG_N, 0, 0, 1>, 6>;
    using op_dec_extended   = opcode<op_sub<uint8_t>,   ExtendedOperand8,   OperandConst<uint8_t, 1>,    compute_flags<FLAG_V|FLAG_Z|FLAG_N, 0, 0, 1>, 7>;

    // EOR
    using op_eora_immediate  = opcode<op_eor,           RegisterA,          ImmediateOperand8,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 2>;
    using op_eora_direct     = opcode<op_eor,           RegisterA,          DirectOperand8,     compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_eora_indexed    = opcode<op_eor,           RegisterA,          IndexedOperand8,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_eora_extended   = opcode<op_eor,           RegisterA,          ExtendedOperand8,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 5>;

    using op_eorb_immediate  = opcode<op_eor,           RegisterB,          ImmediateOperand8,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 2>;
    using op_eorb_direct     = opcode<op_eor,           RegisterB,          DirectOperand8,     compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_eorb_indexed    = opcode<op_eor,           RegisterB,          IndexedOperand8,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_eorb_extended   = opcode<op_eor,           RegisterB,          ExtendedOperand8,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 5>;

    // EXG
    using op_exg_immediate  = opcode<op_exg, ImmediateOperand8, inherent, compute_flags<>, 8>;

    // INC
    using op_inca_inherent  = opcode<op_add<uint8_t>,   RegisterA,          OperandConst<uint8_t, 1>,    compute_flags<FLAG_V|FLAG_Z|FLAG_N>, 2>;
    using op_incb_inherent  = opcode<op_add<uint8_t>,   RegisterB,          OperandConst<uint8_t, 1>,    compute_flags<FLAG_V|FLAG_Z|FLAG_N>, 2>;

    using op_inc_direct     = opcode<op_add<uint8_t>,   DirectOperand8,     OperandConst<uint8_t, 1>,    compute_flags<FLAG_V|FLAG_Z|FLAG_N>, 6>;
    using op_inc_indexed    = opcode<op_add<uint8_t>,   IndexedOperand8,    OperandConst<uint8_t, 1>,    compute_flags<FLAG_V|FLAG_Z|FLAG_N>, 6>;
    using op_inc_extended   = opcode<op_add<uint8_t>,   ExtendedOperand8,   OperandConst<uint8_t, 1>,    compute_flags<FLAG_V|FLAG_Z|FLAG_N>, 7>;

    // JMP - copies the EA in to the PC register
    using op_jmp_direct     = opcode<op_copy<uint16_t>, RegisterPC, DirectEA,   NoFlags16, 3>;
    using op_jmp_indexed    = opcode<op_copy<uint16_t>, RegisterPC, IndexedEA,  NoFlags16, 3>;
    using op_jmp_extended   = opcode<op_copy<uint16_t>, RegisterPC, ExtendedEA, NoFlags16, 4>;

    // JSR (long)
    using op_jsr_direct     = opcode<op_jsr<uint16_t>, RegisterPC, DirectEA,   NoFlags16, 7>;
    using op_jsr_indexed    = opcode<op_jsr<uint16_t>, RegisterPC, IndexedEA,  NoFlags16, 7>;
    using op_jsr_extended   = opcode<op_jsr<uint16_t>, RegisterPC, ExtendedEA, NoFlags16, 8>;

    // BSR (short and long)
    using op_bsr_relative   = opcode<op_jsr<uint8_t>,  RegisterPC, RelativeEA,     NoFlags16, 7>;
    using op_lbsr_relative  = opcode<op_jsr<uint16_t>, RegisterPC, RelativeEALong, NoFlags16, 9>;

    // LD
    using op_lda_immediate  = opcode<op_copy<uint8_t>,   RegisterA,         ImmediateOperand8,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 2>;
    using op_lda_direct     = opcode<op_copy<uint8_t>,   RegisterA,         DirectOperand8,     compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_lda_indexed    = opcode<op_copy<uint8_t>,   RegisterA,         IndexedOperand8,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_lda_extended   = opcode<op_copy<uint8_t>,   RegisterA,         ExtendedOperand8,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 5>;

    using op_ldb_immediate  = opcode<op_copy<uint8_t>,   RegisterB,         ImmediateOperand8,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 2>;
    using op_ldb_direct     = opcode<op_copy<uint8_t>,   RegisterB,         DirectOperand8,     compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_ldb_indexed    = opcode<op_copy<uint8_t>,   RegisterB,         IndexedOperand8,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_ldb_extended   = opcode<op_copy<uint8_t>,   RegisterB,         ExtendedOperand8,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 5>;

    using op_ldd_immediate  = opcode<op_copy<uint16_t>,  RegisterD,         ImmediateOperand16, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 3>;
    using op_ldd_direct     = opcode<op_copy<uint16_t>,  RegisterD,         DirectOperand16,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_ldd_indexed    = opcode<op_copy<uint16_t>,  RegisterD,         IndexedOperand16,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_ldd_extended   = opcode<op_copy<uint16_t>,  RegisterD,         ExtendedOperand16,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;

    using op_lds_immediate  = opcode<op_copy<uint16_t>,  RegisterSP,        ImmediateOperand16, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 4>;
    using op_lds_direct     = opcode<op_copy<uint16_t>,  RegisterSP,        DirectOperand16,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;
    using op_lds_indexed    = opcode<op_copy<uint16_t>,  RegisterSP,        IndexedOperand16,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;
    using op_lds_extended   = opcode<op_copy<uint16_t>,  RegisterSP,        ExtendedOperand16,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 7>;

    using op_ldu_immediate  = opcode<op_copy<uint16_t>,  RegisterUSP,       ImmediateOperand16, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 3>;
    using op_ldu_direct     = opcode<op_copy<uint16_t>,  RegisterUSP,       DirectOperand16,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_ldu_indexed    = opcode<op_copy<uint16_t>,  RegisterUSP,       IndexedOperand16,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_ldu_extended   = opcode<op_copy<uint16_t>,  RegisterUSP,       ExtendedOperand16,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;

    using op_ldx_immediate  = opcode<op_copy<uint16_t>,  RegisterX,         ImmediateOperand16, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 3>;
    using op_ldx_direct     = opcode<op_copy<uint16_t>,  RegisterX,         DirectOperand16,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_ldx_indexed    = opcode<op_copy<uint16_t>,  RegisterX,         IndexedOperand16,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_ldx_extended   = opcode<op_copy<uint16_t>,  RegisterX,         ExtendedOperand16,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;

    using op_ldy_immediate  = opcode<op_copy<uint16_t>,  RegisterY,         ImmediateOperand16, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 4>;
    using op_ldy_direct     = opcode<op_copy<uint16_t>,  RegisterY,         DirectOperand16,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;
    using op_ldy_indexed    = opcode<op_copy<uint16_t>,  RegisterY,         IndexedOperand16,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;
    using op_ldy_extended   = opcode<op_copy<uint16_t>,  RegisterY,         ExtendedOperand16,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 7>;

    // LEA
    using op_leas_indexed   = opcode<op_copy<uint16_t>,  RegisterSP,        IndexedEA,          NoFlags16, 4>;
    using op_leau_indexed   = opcode<op_copy<uint16_t>,  RegisterUSP,       IndexedEA,          NoFlags16,4 >;
    using op_leax_indexed   = opcode<op_copy<uint16_t>,  RegisterX,         IndexedEA,          compute_flags<FLAG_Z, 0, 0, 0, uint16_t>, 4>;
    using op_leay_indexed   = opcode<op_copy<uint16_t>,  RegisterY,         IndexedEA,          compute_flags<FLAG_Z, 0, 0, 0, uint16_t>, 4>;

    // LSL - also updated C and H flags
    using op_lsla_inherent  = opcode<op_lsl,             RegisterA,          inherent,          compute_flags<FLAG_N | FLAG_Z>, 2>;
    using op_lslb_inherent  = opcode<op_lsl,             RegisterB,          inherent,          compute_flags<FLAG_N | FLAG_Z>, 2>;

    using op_lsl_direct     = opcode<op_lsl,             DirectOperand8,     inherent,          compute_flags<FLAG_N | FLAG_Z>, 6>;
    using op_lsl_indexed    = opcode<op_lsl,             IndexedOperand8,    inherent,          compute_flags<FLAG_N | FLAG_Z>, 6>;
    using op_lsl_extended   = opcode<op_lsl,             ExtendedOperand8,   inherent,          compute_flags<FLAG_N | FLAG_Z>, 7>;

    // LSR - also updated C flag
    using op_lsra_inherent  = opcode<op_lsr,             RegisterA,          inherent,          compute_flags<FLAG_Z, 0, FLAG_N>, 2>;
    using op_lsrb_inherent  = opcode<op_lsr,             RegisterB,          inherent,          compute_flags<FLAG_Z, 0, FLAG_N>, 2>;

    using op_lsr_direct     = opcode<op_lsr,             DirectOperand8,     inherent,          compute_flags<FLAG_Z, 0, FLAG_N>, 6>;
    using op_lsr_indexed    = opcode<op_lsr,             IndexedOperand8,    inherent,          compute_flags<FLAG_Z, 0, FLAG_N>, 6>;
    using op_lsr_extended   = opcode<op_lsr,             ExtendedOperand8,   inherent,          compute_flags<FLAG_Z, 0, FLAG_N>, 7>;

    // MUL - special case for flags
    using op_mul_inherent   = opcode<op_mul, RegisterD, inherent, compute_flags<FLAG_Z|FLAG_M, 0, 0, 0, uint16_t>, 11>;

    // NEG
    using op_nega_inherent  = opcode<op_neg,             RegisterA,          inherent,          FlagMathsSub, 2>;
    using op_negb_inherent  = opcode<op_neg,             RegisterB,          inherent,          FlagMathsSub, 2>;

    using op_neg_direct     = opcode<op_neg,             DirectOperand8,     inherent,          FlagMathsSub, 6>;
    using op_neg_indexed    = opcode<op_neg,             IndexedOperand8,    inherent,          FlagMathsSub, 6>;
    using op_neg_extended   = opcode<op_neg,             ExtendedOperand8,   inherent,          FlagMathsSub, 7>;

    // NOP
    using op_nop_inherent   = opcode<op_nop, inherent, inherent, compute_flags<>, 2>;

    // OR
    using op_ora_immediate  = opcode<op_or,             RegisterA,          ImmediateOperand8,  LogicFlags, 2>;
    using op_ora_direct     = opcode<op_or,             RegisterA,          DirectOperand8,     LogicFlags, 4>;
    using op_ora_indexed    = opcode<op_or,             RegisterA,          IndexedOperand8,    LogicFlags, 4>;
    using op_ora_extended   = opcode<op_or,             RegisterA,          ExtendedOperand8,   LogicFlags, 5>;

    using op_orb_immediate  = opcode<op_or,             RegisterB,          ImmediateOperand8,  LogicFlags, 2>;
    using op_orb_direct     = opcode<op_or,             RegisterB,          DirectOperand8,     LogicFlags, 4>;
    using op_orb_indexed    = opcode<op_or,             RegisterB,          IndexedOperand8,    LogicFlags, 4>;
    using op_orb_extended   = opcode<op_or,             RegisterB,          ExtendedOperand8,   LogicFlags, 5>;

    // ORCC - do not update the flags, they are affected by the immediate value
    using op_orcc_immediate = opcode<op_or,             RegisterCC,         ImmediateOperand8, compute_flags<>, 3>;

    // PSHS / PSHU
    using op_pshs_immediate = opcode_count<op_push<reg_sp, reg_usp>, ImmediateOperand8, inherent, compute_flags<>, 5>;
    using op_pshu_immediate = opcode_count<op_push<reg_usp, reg_sp>, ImmediateOperand8, inherent, compute_flags<>, 5>;

    // PULS / PULU
    using op_puls_immediate = opcode_count<op_pull<reg_sp, reg_usp>, ImmediateOperand8, inherent, compute_flags<>, 5>;
    using op_pulu_immediate = opcode_count<op_pull<reg_usp, reg_sp>, ImmediateOperand8, inherent, compute_flags<>, 5>;

    // ROL - special cases for V and C
    using op_rola_inherent  = opcode<op_rol,             RegisterA,          inherent,          compute_flags<FLAG_N | FLAG_Z>, 2>;
    using op_rolb_inherent  = opcode<op_rol,             RegisterB,          inherent,          compute_flags<FLAG_N | FLAG_Z>, 2>;

    using op_rol_direct     = opcode<op_rol,             DirectOperand8,     inherent,          compute_flags<FLAG_N | FLAG_Z>, 6>;
    using op_rol_indexed    = opcode<op_rol,             IndexedOperand8,    inherent,          compute_flags<FLAG_N | FLAG_Z>, 6>;
    using op_rol_extended   = opcode<op_rol,             ExtendedOperand8,   inherent,          compute_flags<FLAG_N | FLAG_Z>, 7>;

    // ROR - specal case for C
    using op_rora_inherent  = opcode<op_ror,             RegisterA,          inherent,          compute_flags<FLAG_N | FLAG_Z>, 2>;
    using op_rorb_inherent  = opcode<op_ror,             RegisterB,          inherent,          compute_flags<FLAG_N | FLAG_Z>, 2>;

    using op_ror_direct     = opcode<op_ror,             DirectOperand8,     inherent,          compute_flags<FLAG_N | FLAG_Z>, 6>;
    using op_ror_indexed    = opcode<op_ror,             IndexedOperand8,    inherent,          compute_flags<FLAG_N | FLAG_Z>, 6>;
    using op_ror_extended   = opcode<op_ror,             ExtendedOperand8,   inherent,          compute_flags<FLAG_N | FLAG_Z>, 7>;

    // RTI
    using op_rti_inherent   = opcode_count<op_rti, RegisterPC, inherent, NoFlags16, 3>; // 6 or 15

    // RTS
    using op_rts_inherent   = opcode<op_rts, RegisterPC, inherent, NoFlags16, 5>;

    // SBC - subtract with carry
    using op_sbca_immediate = opcode<op_sbc,            RegisterA,          ImmediateOperand8,  FlagMaths, 2>;
    using op_sbca_direct    = opcode<op_sbc,            RegisterA,          DirectOperand8,     FlagMaths, 4>;
    using op_sbca_indexed   = opcode<op_sbc,            RegisterA,          IndexedOperand8,    FlagMaths, 4>;
    using op_sbca_extended  = opcode<op_sbc,            RegisterA,          ExtendedOperand8,   FlagMaths, 5>;

    using op_sbcb_immediate = opcode<op_sbc,            RegisterB,          ImmediateOperand8,  FlagMaths, 2>;
    using op_sbcb_direct    = opcode<op_sbc,            RegisterB,          DirectOperand8,     FlagMaths, 4>;
    using op_sbcb_indexed   = opcode<op_sbc,            RegisterB,          IndexedOperand8,    FlagMaths, 4>;
    using op_sbcb_extended  = opcode<op_sbc,            RegisterB,          ExtendedOperand8,   FlagMaths, 5>;

    // SEX sign extend A in to B
    using op_sex_inherent   = opcode<op_sex, RegisterD, RegisterB, compute_flags<0, 0, FLAG_V, 0, uint16_t, uint8_t>, 2>;

    // ST (reverse of LD, the memory locations are update only)
    using op_sta_direct     = opcode<op_copy<uint8_t>,   DirectOperand8_WO,     RegisterA,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_sta_indexed    = opcode<op_copy<uint8_t>,   IndexedOperand8_WO,    RegisterA,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_sta_extended   = opcode<op_copy<uint8_t>,   ExtendedOperand8_WO,   RegisterA,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 5>;

    using op_stb_direct     = opcode<op_copy<uint8_t>,   DirectOperand8_WO,     RegisterB,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_stb_indexed    = opcode<op_copy<uint8_t>,   IndexedOperand8_WO,    RegisterB,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 4>;
    using op_stb_extended   = opcode<op_copy<uint8_t>,   ExtendedOperand8_WO,   RegisterB,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>, 5>;

    using op_std_direct     = opcode<op_copy<uint16_t>,  DirectOperand16_WO,    RegisterD,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_std_indexed    = opcode<op_copy<uint16_t>,  IndexedOperand16_WO,   RegisterD,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_std_extended   = opcode<op_copy<uint16_t>,  ExtendedOperand16_WO,  RegisterD,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;

    using op_sts_direct     = opcode<op_copy<uint16_t>,  DirectOperand16_WO,    RegisterSP,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;
    using op_sts_indexed    = opcode<op_copy<uint16_t>,  IndexedOperand16_WO,   RegisterSP,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;
    using op_sts_extended   = opcode<op_copy<uint16_t>,  ExtendedOperand16_WO,  RegisterSP,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 7>;

    using op_stu_direct     = opcode<op_copy<uint16_t>,  DirectOperand16_WO,    RegisterUSP, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_stu_indexed    = opcode<op_copy<uint16_t>,  IndexedOperand16_WO,   RegisterUSP, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_stu_extended   = opcode<op_copy<uint16_t>,  ExtendedOperand16_WO,  RegisterUSP, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;

    using op_stx_direct     = opcode<op_copy<uint16_t>,  DirectOperand16_WO,    RegisterX,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_stx_indexed    = opcode<op_copy<uint16_t>,  IndexedOperand16_WO,   RegisterX,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 5>;
    using op_stx_extended   = opcode<op_copy<uint16_t>,  ExtendedOperand16_WO,  RegisterX,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;

    using op_sty_direct     = opcode<op_copy<uint16_t>,  DirectOperand16_WO,    RegisterY,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;
    using op_sty_indexed    = opcode<op_copy<uint16_t>,  IndexedOperand16_WO,   RegisterY,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 6>;
    using op_sty_extended   = opcode<op_copy<uint16_t>,  ExtendedOperand16_WO,  RegisterY,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, 0, uint16_t>, 7>;

    // SUB
    using op_suba_immediate = opcode<op_sub<uint8_t>,  RegisterA,         ImmediateOperand8,  FlagMathsSub, 2>;
    using op_suba_direct    = opcode<op_sub<uint8_t>,  RegisterA,         DirectOperand8,     FlagMathsSub, 4>;
    using op_suba_indexed   = opcode<op_sub<uint8_t>,  RegisterA,         IndexedOperand8,    FlagMathsSub, 4>;
    using op_suba_extended  = opcode<op_sub<uint8_t>,  RegisterA,         ExtendedOperand8,   FlagMathsSub, 5>;

    using op_subb_immediate = opcode<op_sub<uint8_t>,  RegisterB,         ImmediateOperand8,  FlagMathsSub, 2>;
    using op_subb_direct    = opcode<op_sub<uint8_t>,  RegisterB,         DirectOperand8,     FlagMathsSub, 4>;
    using op_subb_indexed   = opcode<op_sub<uint8_t>,  RegisterB,         IndexedOperand8,    FlagMathsSub, 4>;
    using op_subb_extended  = opcode<op_sub<uint8_t>,  RegisterB,         ExtendedOperand8,   FlagMathsSub, 5>;

    using op_subd_immediate = opcode<op_sub<uint16_t>, RegisterD,         ImmediateOperand16, FlagMaths16Sub, 4>;
    using op_subd_direct    = opcode<op_sub<uint16_t>, RegisterD,         DirectOperand16,    FlagMaths16Sub, 6>;
    using op_subd_indexed   = opcode<op_sub<uint16_t>, RegisterD,         IndexedOperand16,   FlagMaths16Sub, 6>;
    using op_subd_extended  = opcode<op_sub<uint16_t>, RegisterD,         ExtendedOperand16,  FlagMaths16Sub, 7>;

    // SWI
    using op_swi1_inherent  = opcode_count<op_swi<SWI1_VECTOR, true>, RegisterPC, inherent, NoFlags16, 7>;
    using op_swi2_inherent  = opcode_count<op_swi<SWI2_VECTOR>,       RegisterPC, inherent, NoFlags16, 8>;
    using op_swi3_inherent  = opcode_count<op_swi<SWI3_VECTOR>,       RegisterPC, inherent, NoFlags16, 8>;

    // SYNC
    using op_sync_inherent  = opcode<op_sync, inherent, inherent, compute_flags<>, 4>;

    // TFR
    using op_tfr_immediate  = opcode<op_tfr, ImmediateOperand8, inherent, compute_flags<>, 6>;

    // TST - test
    using op_tsta_inherent  = opcode<op_tst, RegisterA_RO,                        inherent, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>, 2>;
    using op_tstb_inherent  = opcode<op_tst, RegisterB_RO,                        inherent, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>, 2>;

    // use read-only memory operands
    using op_tst_direct     = opcode<op_tst, MemoryOperand<uint8_t, direct, 0>,   inherent, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>, 6>;
    using op_tst_indexed    = opcode<op_tst, MemoryOperand<uint8_t, indexed, 0>,  inherent, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>, 6>;
    using op_tst_extended   = opcode<op_tst, MemoryOperand<uint8_t, extended, 0>, inherent, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>, 7>;

    // Branches
    using op_bra_inherent   = opcode_count<op_bra_short<op_bra_always>,           RegisterPC,   inherent,  NoFlags16, 3>;
    using op_brn_inherent   = opcode_count<op_bra_short<op_bra_always, true>,     RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bcs_inherent   = opcode_count<op_bra_short<op_bra_carry>,            RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bcc_inherent   = opcode_count<op_bra_short<op_bra_carry, true>,      RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bhi_inherent   = opcode_count<op_bra_short<op_bra_less>,             RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bls_inherent   = opcode_count<op_bra_short<op_bra_less, true>,       RegisterPC,   inherent,  NoFlags16, 3>;
    using op_beq_inherent   = opcode_count<op_bra_short<op_bra_equal>,            RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bne_inherent   = opcode_count<op_bra_short<op_bra_equal, true>,      RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bgt_inherent   = opcode_count<op_bra_short<op_bra_less_eq, true>,    RegisterPC,   inherent,  NoFlags16, 3>;
    using op_ble_inherent   = opcode_count<op_bra_short<op_bra_less_eq>,          RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bge_inherent   = opcode_count<op_bra_short<op_bra_less_than, true>,  RegisterPC,   inherent,  NoFlags16, 3>;
    using op_blt_inherent   = opcode_count<op_bra_short<op_bra_less_than>,        RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bpl_inherent   = opcode_count<op_bra_short<op_bra_plus>,             RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bmi_inherent   = opcode_count<op_bra_short<op_bra_plus, true>,       RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bvs_inherent   = opcode_count<op_bra_short<op_bra_overflow>,         RegisterPC,   inherent,  NoFlags16, 3>;
    using op_bvc_inherent   = opcode_count<op_bra_short<op_bra_overflow, true>,   RegisterPC,   inherent,  NoFlags16, 3>;

    using op_lbra_inherent  = opcode_count<op_bra_long<op_bra_always>,            RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbrn_inherent  = opcode_count<op_bra_long<op_bra_always, true>,      RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbcs_inherent  = opcode_count<op_bra_long<op_bra_carry>,             RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbcc_inherent  = opcode_count<op_bra_long<op_bra_carry, true>,       RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbhi_inherent  = opcode_count<op_bra_long<op_bra_less>,              RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbls_inherent  = opcode_count<op_bra_long<op_bra_less, true>,        RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbeq_inherent  = opcode_count<op_bra_long<op_bra_equal>,             RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbne_inherent  = opcode_count<op_bra_long<op_bra_equal, true>,       RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbgt_inherent  = opcode_count<op_bra_long<op_bra_less_eq, true>,     RegisterPC,   inherent, NoFlags16, 5>;
    using op_lble_inherent  = opcode_count<op_bra_long<op_bra_less_eq>,           RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbge_inherent  = opcode_count<op_bra_long<op_bra_less_than, true>,   RegisterPC,   inherent, NoFlags16, 5>;
    using op_lblt_inherent  = opcode_count<op_bra_long<op_bra_less_than>,         RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbpl_inherent  = opcode_count<op_bra_long<op_bra_plus>,              RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbmi_inherent  = opcode_count<op_bra_long<op_bra_plus, true>,        RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbvs_inherent  = opcode_count<op_bra_long<op_bra_overflow>,          RegisterPC,   inherent, NoFlags16, 5>;
    using op_lbvc_inherent  = opcode_count<op_bra_long<op_bra_overflow, true>,    RegisterPC,   inherent, NoFlags16, 5>;

    // RESET - Undocumented
    using op_reset_inherent = opcode<op_reset>;

public:
    M6809();

    // Reset the CPU to it's default state, clearing the registers and setting the PC to the reset vector
    void Reset();

    // Set callbacks for read and write, must be a static function
    void SetReadCallback(read_callback_t func, intptr_t ref);
    void SetWriteCallback(write_callback_t func, intptr_t ref);

    // Exceture one instruction and updated the number of cycles that it took
    m6809_error_t Execute(uint64_t &cycles, m6809_interrupt_t irq=NONE);

    Registers &getRegisters() { return registers; }
};

#endif //VECTREXIA_M6809_H
