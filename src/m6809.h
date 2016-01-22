#ifndef VECTREXIA_M6809_H
#define VECTREXIA_M6809_H

#include <stdint.h>
#include <functional>
#include <array>
#include <iostream>
#include <type_traits>
#include <memory>
#include <ostream>

enum m6809_error_t {
    E_SUCCESS = 0,
    E_UNKNOWN_OPCODE = -1,
    E_UNKNOWN_OPCODE_PAGE1 = -2,
    E_UNKNOWN_OPCODE_PAGE2 = -3,
    E_ILLEGAL_INDEXING_MODE = -4,
};

enum flag_mask_t {
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

struct M6809Registers;

class M6809
{
    using read_callback_t = uint8_t (*)(intptr_t, uint16_t);
    using write_callback_t = void (*)(intptr_t, uint16_t, uint8_t);
    using opcode_handler_t = void (*)(M6809 &, int &);

    static const uint16_t RESET_VECTOR = 0xfffe;

    struct M6809Registers
    {
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
            uint16_t D;
        };
        uint16_t X, Y;
        uint16_t PC;
        uint16_t USP, SP;   // User Stack Pointer and Stack Pointer
        uint8_t DP;         // Direct Page register
        union
        {
            uint8_t CC;         // Condition Code
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

    } registers;


    // memory accessors
    //   callbacks
    read_callback_t read_callback_func;
    intptr_t read_callback_ref;
    write_callback_t write_callback_func;
    intptr_t write_callback_ref;

    bool wait_for_intterupt = false;

    uint16_t *index_mode_register_table[4] = {
            &registers.X,
            &registers.Y,
            &registers.USP,
            &registers.SP
    };

    intptr_t exg_register_table[0xc] = {
            reinterpret_cast<intptr_t >(&registers.D),
            reinterpret_cast<intptr_t >(&registers.X),
            reinterpret_cast<intptr_t >(&registers.Y),
            reinterpret_cast<intptr_t >(&registers.USP),
            0, 0,
            reinterpret_cast<intptr_t >(&registers.SP),
            reinterpret_cast<intptr_t >(&registers.A),
            reinterpret_cast<intptr_t >(&registers.B),
            reinterpret_cast<intptr_t >(&registers.CC),
            reinterpret_cast<intptr_t >(&registers.DP),
    };

    inline uint8_t Read8(const uint16_t &addr)
    {
        return read_callback_func(read_callback_ref, addr);
    }

    inline uint16_t Read16(const uint16_t &addr)
    {
        return (uint16_t) Read8(addr) | (uint16_t) Read8((uint16_t) (addr + 1)) << 8;
    }

    inline void Write8(const uint16_t &addr, const uint8_t &data)
    {
        write_callback_func(write_callback_ref, addr, data);
    }

    inline void Write16(const uint16_t &addr, const uint16_t &data)
    {
        Write8(addr, (uint8_t) (data & 0xff));
        Write8((uint16_t) (addr + 1), (uint8_t) (data >> 8));
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
    struct direct { uint16_t operator()(M6809& cpu) { return (cpu.registers.DP << 8) | cpu.ReadPC8(); } };
    struct extended { uint16_t operator()(M6809& cpu) { return cpu.ReadPC16(); } };
    struct indexed { uint16_t operator()(M6809& cpu) {
            uint16_t ea;
            uint8_t post_byte = cpu.ReadPC8();

            uint16_t &reg = *cpu.index_mode_register_table[(post_byte >> 5) & 0x03];  // bits 5+6

            if (post_byte & 0x80)
            {
                // (+/- 4 bit offset),R
                return reg + (int8_t)(post_byte & 0x1f);
            }
            else
            {
                switch (post_byte & 0xf)
                {
                    case 0:
                    case 1:
                        // ,R+ | ,R++
                        ea = reg;
                        // register is incremented by 1 or 2
                        reg += 1 + (post_byte & 1);
                        break;
                    case 2:
                    case 3:
                        // ,-R | ,--R
                        ea = reg;
                        // register is decremented by 1 or 2
                        reg += 1 + (post_byte & 1);
                        break;
                    case 4:
                        // ,R
                        ea = reg;
                        break;
                    case 5:
                        // (+/- B), R
                        ea = reg + (int8_t)cpu.registers.B;
                        break;
                    case 6:
                        // (+/- A), R
                        ea = reg + (int8_t)cpu.registers.A;
                        break;
                    case 8:
                        // (+/- 7 bit offset), R
                        ea = reg + (int16_t)cpu.ReadPC8();
                        break;
                    case 9:
                        // (+/- 15 bit offset), R
                        ea = reg + (int16_t)cpu.ReadPC16();
                        break;
                    case 0xb:
                        // (+/- D), R
                        ea = reg + (int16_t)cpu.registers.D;
                        break;
                    case 0xc:
                        // (+/- 7 bit offset), PC
                        ea = reg + (int16_t)cpu.ReadPC8();
                        break;
                    case 0xd:
                        // (+/- 15 bit offset), PC
                        ea = reg + (int16_t)cpu.ReadPC16();
                        break;
                    case 0xf:
                        ea = cpu.ReadPC16();
                        break;
                    default:
                        // Illegal
                        return 0;
                }
                // indirect mode
                if ((post_byte >> 4) & 1)
                {
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
     */
    struct Operand
    {
        M6809 &cpu;
        Operand(M6809 &cpu) : cpu(cpu) { }
    };

    template <typename T, typename Fn, int RW=1>
    struct MemoryOperand : Operand
    {
        MemoryOperand(M6809 &cpu) : Operand(cpu) {}

        inline T operator ()(uint16_t &addr) {
            addr = Fn()(cpu);
            return (sizeof(T) == 1) ? cpu.Read8(addr) : cpu.Read16(addr);
        }
        void update(uint16_t addr, T data) {
            if (RW)
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
    struct MemoryOperand<T, immediate, RW> : Operand
    {
        MemoryOperand(M6809 &cpu) : Operand(cpu) {}

        inline T operator ()(uint16_t &addr) {
            if (sizeof(T) == 1)
                return cpu.ReadPC8();
            else
                return cpu.ReadPC16();
        }
        inline void update(uint16_t addr, T data) { }
    };

    template <typename T, typename Fn, int RW=1>
    struct Register : Operand
    {
        Register(M6809 &cpu) : Operand(cpu) { }

        inline T &operator() (uint16_t &addr) {
            return Fn()(cpu, addr);
        }
        inline void update(uint16_t addr, T data) {
            if (RW)
            {
                Fn()(cpu, addr) = data;
            }
        }
    };

    template <typename Fn>
    struct OperandEA : Operand
    {
        OperandEA(M6809 &cpu) : Operand(cpu) {}

        inline uint16_t operator ()(uint16_t &addr) {
            return addr;
        }
        void update(uint16_t addr, uint16_t data) {}
    };

    /*
     * Alaises for Registers and memory addressing modes
     */
    using RegisterA = Register<uint8_t, reg_a>;
    using RegisterB = Register<uint8_t, reg_b>;
    using RegisterD = Register<uint16_t, reg_d>;
    using RegisterX = Register<uint16_t, reg_x>;
    using RegisterY = Register<uint16_t, reg_y>;
    using RegisterSP = Register<uint16_t, reg_sp>;
    using RegisterUSP = Register<uint16_t, reg_usp>;
    using RegisterCC = Register<uint8_t, reg_cc>;
    using RegisterPC = Register<uint16_t, reg_pc>
    ;
    // read-only version
    using RegisterA_RO    = Register<uint8_t,  reg_a,   0>;
    using RegisterB_RO    = Register<uint8_t,  reg_b,   0>;
    using RegisterD_RO    = Register<uint16_t, reg_d,   0>;
    using RegisterX_RO    = Register<uint16_t, reg_x,   0>;
    using RegisterY_RO    = Register<uint16_t, reg_y,   0>;
    using RegisterCC_RO   = Register<uint8_t,  reg_cc,  0>;
    using RegisterSP_RO   = Register<uint16_t,  reg_sp,  0>;
    using RegisterUSP_RO  = Register<uint16_t,  reg_usp, 0>;

    using ImmediateOperand8 = MemoryOperand<uint8_t, immediate>;
    using ImmediateOperand16 = MemoryOperand<uint16_t, immediate>;
    using DirectOperand8 = MemoryOperand<uint8_t, direct>;
    using DirectOperand16 = MemoryOperand<uint16_t, direct>;
    using ExtendedOperand8 = MemoryOperand<uint8_t, extended>;
    using ExtendedOperand16 = MemoryOperand<uint16_t, extended>;
    using IndexedOperand8 = MemoryOperand<uint8_t, indexed>;
    using IndexedOperand16 = MemoryOperand<uint16_t, indexed>;

    using DirectEA = OperandEA<direct>;
    using IndexedEA = OperandEA<indexed>;
    using ExtendedEA = OperandEA<extended>;

    /*
     * Calculate flags
     */
    template <int FlagUpdateMask=0, int FlagSetMask=0, int FlagClearMask=0, typename T=uint8_t, typename T2=T>
    struct compute_flags
    {
        inline void operator() (M6809 &cpu, T &result, T &operand_a, T2 &operand_b)
        {
            if (FlagClearMask) cpu.registers.CC &= ~FlagClearMask;
            if (FlagSetMask) cpu.registers.CC |= FlagSetMask;

            if (FlagUpdateMask & FLAG_Z) cpu.ComputeZeroFlag<T>(cpu.registers.CC, result);
            if (FlagUpdateMask & FLAG_N) cpu.ComputeNegativeFlag(cpu.registers.CC, result);
            if (FlagUpdateMask & FLAG_H) cpu.ComputeHalfCarryFlag(cpu.registers.CC, operand_a, operand_b, result);
            if (FlagUpdateMask & FLAG_V) cpu.ComputeOverflowFlag<T>(cpu.registers.CC, operand_a, operand_b, result);
            if (FlagUpdateMask & FLAG_C) cpu.ComputeCarryFlag<T>(cpu.registers.CC, operand_a, operand_b, result);
        }
    };

    // Aliases for common flags
    using FlagMaths = compute_flags<FLAGS_MATH>;
    using FlagMaths16 = compute_flags<FLAGS_MATH, 0, 0, uint16_t>;
    using LogicFlags = compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>;
    using NoFlags16 = compute_flags<0,0,0,uint16_t>;

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
            if (sizeof(T) == 8)
                return operand_a - static_cast<int8_t>(operand_b);
            else
                return operand_a - static_cast<int16_t>(operand_b);
        }
    };

    struct op_eor { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b) { return operand_a ^ operand_b; } };
    struct op_and { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b) { return operand_a & operand_b; } };
    struct op_or { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b) { return operand_a | operand_b; } };
    struct op_sex { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b)
        { return (uint8_t) ((~(operand_b & 0x80) + 1) | (operand_b & 0xff)); }
    };
    // Push PC on to the SP, and set the PC to the operand
    struct op_jsr {
        uint8_t operator() (M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b)
        {
            cpu.Push16(cpu.registers.SP, operand_a);
            return operand_b;
        }
    };

    // store/load, M <= Register or Register <= Memory
    template <typename T>
    struct op_copy { T operator() (const M6809& cpu, const T &operand_a, const T &operand_b) { return operand_b; } };

    // This is a special case where the operation sets a pseudo flag to tell the cpu to wait for an interrupt
    struct op_cwai {
        uint8_t operator() (M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b) {
            cpu.wait_for_intterupt = true;
            return operand_a ^ operand_b;
        }
    };


    // one operand
    struct op_dec { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return (uint8_t) (operand - 1); } };
    struct op_inc { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return (uint8_t) (operand + 1); } };
    struct op_clr { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return 0; } };
    struct op_asr { uint8_t operator() (const M6809& cpu, const uint8_t &operand)
        { return (uint8_t) (((operand >> 1) & 0x7f) | (operand & 0x80)); }
    };
    struct op_com { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return ~operand; } };
    struct op_lsl { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return operand << 1; } };
    struct op_lsr { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return operand >> 1; } };
    struct op_neg { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return (uint8_t) (~operand + 1); } };
    struct op_tst { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return operand; } };
    struct op_ror { uint8_t operator() (const M6809& cpu, const uint8_t &operand)
        { return (uint8_t) (((operand >> 1) & 0x7f) | (cpu.registers.flags.C << 7)); }
    };
    struct op_rol { uint8_t operator() (const M6809& cpu, const uint8_t &operand)
        { return (operand << 1) | cpu.registers.flags.C; }
    };
    struct op_rts { uint16_t operator() (M6809& cpu, const uint8_t &operand) { return cpu.Pull16(cpu.registers.SP); } };
    struct op_rti {
        // pull the registers and then the pc
        uint16_t operator() (M6809& cpu, const uint8_t &operand) {
            if (cpu.registers.flags.E)
                op_pull<reg_sp>()(cpu, 0xff);
            else
                op_pull<reg_sp>()(cpu, 0x81);
            return cpu.Pull16(cpu.registers.SP);
        }
    };

    // assign one register to the other and resize
    template <typename T1, typename T2>
    struct op_reg_assign {
        void operator() (T1 &reg_1, const T2 &reg_2)
        {
            if (sizeof(T1) > sizeof(T2))
                reg_1 = (uint8_t)(reg_2 >> 8);
            else if (sizeof(T1) < sizeof(T2))
                reg_1 = reg_2 | 0xFF00;
            else
                reg_1 = reg_2;
        }
    };

    // swap two registers
    template <typename T1, typename T2>
    struct op_swap_registers {
        void operator() (T1 &reg_1, T2 &reg_2)
        {
            T1 temp = reg_1;
            op_reg_assign<T2, T1>()(reg_2, temp);
            op_reg_assign<T1, T2>()(reg_1, reg_2);
        }
    };

    struct op_exg {
        uint8_t operator() (const M6809& cpu, const uint8_t &operand) {
            intptr_t reg_1 = cpu.exg_register_table[operand & 0xf];
            intptr_t reg_2 = cpu.exg_register_table[(operand >> 4) & 0xf];

            auto a_is_8 = (operand & 0x80) >> 7;
            auto b_is_8 = (operand & 0x8) >> 3;

            if (a_is_8 && b_is_8)
                op_swap_registers<uint8_t, uint8_t>()(reinterpret_cast<uint8_t&>(reg_1), reinterpret_cast<uint8_t&>(reg_2));
            else if (a_is_8 && !b_is_8)
                op_swap_registers<uint8_t, uint16_t>()(reinterpret_cast<uint8_t&>(reg_1), reinterpret_cast<uint16_t&>(reg_2));
            else if (!a_is_8 && !b_is_8)
                op_swap_registers<uint16_t, uint16_t>()(reinterpret_cast<uint16_t&>(reg_1), reinterpret_cast<uint16_t&>(reg_2));
            else if (!a_is_8 && b_is_8)
                op_swap_registers<uint16_t, uint8_t>()(reinterpret_cast<uint16_t&>(reg_1), reinterpret_cast<uint8_t&>(reg_2));
            return 0;
        }
    };

    struct op_tfr {
        uint8_t operator() (const M6809& cpu, const uint8_t &operand) {
            intptr_t reg_1 = cpu.exg_register_table[operand & 0xf];
            intptr_t reg_2 = cpu.exg_register_table[(operand >> 4) & 0xf];

            auto a_is_8 = (operand & 0x80) >> 7;
            auto b_is_8 = (operand & 0x8) >> 3;

            if (a_is_8 && b_is_8)
                op_reg_assign<uint8_t, uint8_t>()(reinterpret_cast<uint8_t&>(reg_1), reinterpret_cast<uint8_t&>(reg_2));
            else if (a_is_8 && !b_is_8)
                op_reg_assign<uint8_t, uint16_t>()(reinterpret_cast<uint8_t&>(reg_1), reinterpret_cast<uint16_t&>(reg_2));
            else if (!a_is_8 && !b_is_8)
                op_reg_assign<uint16_t, uint8_t>()(reinterpret_cast<uint16_t&>(reg_1), reinterpret_cast<uint8_t&>(reg_2));
            else if (!a_is_8 && b_is_8)
                op_reg_assign<uint16_t, uint16_t>()(reinterpret_cast<uint16_t&>(reg_1), reinterpret_cast<uint16_t&>(reg_2));
        }
    };

    // push register on to the stack
    template <typename SP>
    struct op_push
    {
        uint8_t operator() (M6809& cpu, const uint8_t &operand)
        {
            // operand contains a bitmask of the registers to push
            auto sp = SP()(cpu, 0);
            if (operand & REG_CC)
            {
                cpu.Push8(sp, cpu.registers.CC);
            }
            if (operand & REG_A)
            {
                cpu.Push8(sp, cpu.registers.A);
            }
            if (operand & REG_B)
            {
                cpu.Push8(sp, cpu.registers.B);
            }
            if (operand & REG_DP)
            {
                cpu.Push8(sp, cpu.registers.DP);
            }
            if (operand & REG_X)
            {
                cpu.Push16(sp, cpu.registers.X);
            }
            if (operand & REG_Y)
            {
                cpu.Push16(sp, cpu.registers.Y);
            }
            if (operand & REG_SP)
            {
                cpu.Push16(sp, sp);
            }
            if (operand & REG_PC)
            {
                cpu.Push16(sp, cpu.registers.PC);
            }
            return 0;
        }
    };

    // pull register from the stack
    template <typename SP>
    struct op_pull
    {
        uint8_t operator() (M6809& cpu, const uint8_t &operand)
        {
            // operand contains a bitmask of the registers to push
            auto sp = SP()(cpu, 0);
            if (operand & REG_CC)
            {
                cpu.registers.CC = cpu.Pull8(sp);
            }
            if (operand & REG_A)
            {
                cpu.registers.A = cpu.Pull8(sp);
            }
            if (operand & REG_B)
            {
                cpu.registers.B = cpu.Pull8(sp);
            }
            if (operand & REG_DP)
            {
                cpu.registers.DP = cpu.Pull8(sp);
            }
            if (operand & REG_X)
            {
                cpu.registers.X = cpu.Pull16(sp);
            }
            if (operand & REG_Y)
            {
                cpu.registers.Y = cpu.Pull16(sp);
            }
            if (operand & REG_SP)
            {
                sp = cpu.Pull16(sp);
            }
            if (operand & REG_PC)
            {
                cpu.registers.PC = cpu.Pull16(sp);
            }
            return 0;
        }
    };

    // no operands
    struct op_nop { uint8_t operator() (const M6809& cpu) { return 0u; } };

    /*
     * Opcode Templates
     */
    template<typename Fn, typename OpA=inherent, typename OpB=inherent, typename Flags=compute_flags<>>
    struct opcode
    {
        void operator() (M6809& cpu, int &cycles)
        {
            uint16_t operand_addr_a = 0;
            uint16_t operand_addr_b = 0;

            auto operand_a = OpA(cpu);
            auto operand_b = OpB(cpu);

            // gets the value for the operand and updates the address
            auto operand_a_value = operand_a(operand_addr_a);
            auto operand_b_value = operand_b(operand_addr_b);

            auto result = Fn()(cpu, operand_a_value, operand_b_value);

            Flags()(cpu, result, operand_a_value, operand_b_value);

            operand_a.update(operand_addr_a, result);
        }
    };

    template<typename Fn, typename OpA, typename Flags>
    struct opcode<Fn, OpA, inherent, Flags>
    {
        void operator() (M6809& cpu, int &cycles)
        {
            uint16_t operand_addr = 0;
            auto operand = OpA(cpu);
            auto operand_value = operand(operand_addr);
            auto result = Fn()(cpu, operand_value);
            decltype(operand_value) zero = 0;

            Flags()(cpu, result, zero, operand_value);

            operand.update(operand_addr, result);
        }
    };

    template<typename Fn, typename Flags>
    struct opcode<Fn, inherent, inherent, Flags>
    {
        void operator() (M6809& cpu, int &cycles)
        {
            auto result = Fn()(cpu);
            decltype(result) zero = 0;
            Flags()(cpu, result, zero, zero);
        }
    };

    template<typename Op>
    static void opcodewrap (M6809& cpu, int &cycles)
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
    using op_abx_inherent = opcode<op_add<uint16_t, uint8_t>,    RegisterX,          RegisterB,          compute_flags<0, 0, 0, uint16_t, uint8_t>>;

    // ADC - ADD with carry
    using op_adca_immediate = opcode<op_adc,            RegisterA,          ImmediateOperand8,  FlagMaths>;
    using op_adca_direct    = opcode<op_adc,            DirectOperand8,     RegisterA,          FlagMaths>;
    using op_adca_indexed   = opcode<op_adc,            IndexedOperand8,    RegisterA,          FlagMaths>;
    using op_adca_extended  = opcode<op_adc,            ExtendedOperand8,   RegisterA,          FlagMaths>;

    using op_adcb_immediate = opcode<op_adc,            RegisterB,          ImmediateOperand8,  FlagMaths>;
    using op_adcb_direct    = opcode<op_adc,            DirectOperand8,     RegisterB,          FlagMaths>;
    using op_adcb_indexed   = opcode<op_adc,            IndexedOperand8,    RegisterB,          FlagMaths>;
    using op_adcb_extended  = opcode<op_adc,            ExtendedOperand8,   RegisterB,          FlagMaths>;

    // ADD
    using op_adda_immediate = opcode<op_add<uint8_t>,   RegisterA,          ImmediateOperand8,  FlagMaths>;
    using op_adda_direct    = opcode<op_add<uint8_t>,   DirectOperand8,     RegisterA,          FlagMaths>;
    using op_adda_indexed   = opcode<op_add<uint8_t>,   IndexedOperand8,    RegisterA,          FlagMaths>;
    using op_adda_extended  = opcode<op_add<uint8_t>,   ExtendedOperand8,   RegisterA,          FlagMaths>;

    using op_addb_immediate = opcode<op_add<uint8_t>,   RegisterB,          ImmediateOperand8,  FlagMaths>;
    using op_addb_direct    = opcode<op_add<uint8_t>,   DirectOperand8,     RegisterB,          FlagMaths>;
    using op_addb_indexed   = opcode<op_add<uint8_t>,   IndexedOperand8,    RegisterB,          FlagMaths>;
    using op_addb_extended  = opcode<op_add<uint8_t>,   ExtendedOperand8,   RegisterB,          FlagMaths>;

    using op_addd_immediate = opcode<op_add<uint16_t>,  RegisterD,          ImmediateOperand16, FlagMaths16>;
    using op_addd_direct    = opcode<op_add<uint16_t>,  DirectOperand16,    RegisterD,          FlagMaths16>;
    using op_addd_indexed   = opcode<op_add<uint16_t>,  IndexedOperand16,   RegisterD,          FlagMaths16>;
    using op_addd_extended  = opcode<op_add<uint16_t>,  ExtendedOperand16,  RegisterD,          FlagMaths16>;

    // AND
    using op_anda_immediate = opcode<op_and,            RegisterA,          ImmediateOperand8,  LogicFlags>;
    using op_anda_direct    = opcode<op_and,            DirectOperand8,     RegisterA,          LogicFlags>;
    using op_anda_indexed   = opcode<op_and,            IndexedOperand8,    RegisterA,          LogicFlags>;
    using op_anda_extended  = opcode<op_and,            ExtendedOperand8,   RegisterA,          LogicFlags>;

    using op_andb_immediate = opcode<op_and,            RegisterB,          ImmediateOperand8,  LogicFlags>;
    using op_andb_direct    = opcode<op_and,            DirectOperand8,     RegisterB,          LogicFlags>;
    using op_andb_indexed   = opcode<op_and,            IndexedOperand8,    RegisterB,          LogicFlags>;
    using op_andb_extended  = opcode<op_and,            ExtendedOperand8,   RegisterB,          LogicFlags>;

    // ANDCC - do not update the flags, they are affected by the immediate value
    using op_andcc_immediate = opcode<op_and,           RegisterCC,         ImmediateOperand8>;

    // ASL (see LSL)
    // ASR
    using op_asra_inherent = opcode<op_asr,             RegisterA,          inherent,           compute_flags<FLAGS_MATH & ~(FLAG_V)>>;
    using op_asrb_inherent = opcode<op_asr,             RegisterB,          inherent,           compute_flags<FLAGS_MATH & ~(FLAG_V)>>;

    using op_asr_direct    = opcode<op_asr,             DirectOperand8,     inherent,           compute_flags<FLAGS_MATH & ~(FLAG_V)>>;
    using op_asr_indexed   = opcode<op_asr,             IndexedOperand8,    inherent,           compute_flags<FLAGS_MATH & ~(FLAG_V)>>;
    using op_asr_extended  = opcode<op_asr,             ExtendedOperand8,   inherent,           compute_flags<FLAGS_MATH & ~(FLAG_V)>>;

    // BIT - bitwise AND, update flags but don't store the result
    using op_bita_immediate = opcode<op_and, RegisterA_RO, ImmediateOperand8, LogicFlags>;
    using op_bita_direct    = opcode<op_and, RegisterA_RO, DirectOperand8,    LogicFlags>;
    using op_bita_indexed   = opcode<op_and, RegisterA_RO, IndexedOperand8,   LogicFlags>;
    using op_bita_extended  = opcode<op_and, RegisterA_RO, ExtendedOperand8,  LogicFlags>;

    using op_bitb_immediate = opcode<op_and, RegisterB_RO, ImmediateOperand8, LogicFlags>;
    using op_bitb_direct    = opcode<op_and, RegisterB_RO, DirectOperand8,    LogicFlags>;
    using op_bitb_indexed   = opcode<op_and, RegisterB_RO, IndexedOperand8,   LogicFlags>;
    using op_bitb_extended  = opcode<op_and, RegisterB_RO, ExtendedOperand8,  LogicFlags>;

    // CLR
    using op_clra_inherent = opcode<op_clr,             RegisterA,          inherent,           compute_flags<0, FLAG_Z, FLAG_N|FLAG_V|FLAG_C>>;
    using op_clrb_inherent = opcode<op_clr,             RegisterB,          inherent,           compute_flags<0, FLAG_Z, FLAG_N|FLAG_V|FLAG_C>>;

    using op_clr_direct    = opcode<op_clr,             DirectOperand8,     inherent,           compute_flags<0, FLAG_Z, FLAG_N|FLAG_V|FLAG_C>>;
    using op_clr_indexed   = opcode<op_clr,             IndexedOperand8,    inherent,           compute_flags<0, FLAG_Z, FLAG_N|FLAG_V|FLAG_C>>;
    using op_clr_extended  = opcode<op_clr,             ExtendedOperand8,   inherent,           compute_flags<0, FLAG_Z, FLAG_N|FLAG_V|FLAG_C>>;

    // CMP
    using op_cmpa_immediate = opcode<op_sub<uint8_t>,   RegisterA_RO,       ImmediateOperand8,  FlagMaths>;
    using op_cmpa_direct =    opcode<op_sub<uint8_t>,   RegisterA_RO,       DirectOperand8,     FlagMaths>;
    using op_cmpa_indexed =   opcode<op_sub<uint8_t>,   RegisterA_RO,       IndexedOperand8,    FlagMaths>;
    using op_cmpa_extended =  opcode<op_sub<uint8_t>,   RegisterA_RO,       ExtendedOperand8,   FlagMaths>;

    using op_cmpb_immediate = opcode<op_sub<uint8_t>,   RegisterB_RO,       ImmediateOperand8,  FlagMaths>;
    using op_cmpb_direct =    opcode<op_sub<uint8_t>,   RegisterB_RO,       DirectOperand8,     FlagMaths>;
    using op_cmpb_indexed =   opcode<op_sub<uint8_t>,   RegisterB_RO,       IndexedOperand8,    FlagMaths>;
    using op_cmpb_extended =  opcode<op_sub<uint8_t>,   RegisterB_RO,       ExtendedOperand8,   FlagMaths>;

    using op_cmpd_immediate = opcode<op_sub<uint16_t>,   RegisterD_RO,       ImmediateOperand16,  FlagMaths16>;
    using op_cmpd_direct =    opcode<op_sub<uint16_t>,   RegisterD_RO,       DirectOperand16,     FlagMaths16>;
    using op_cmpd_indexed =   opcode<op_sub<uint16_t>,   RegisterD_RO,       IndexedOperand16,    FlagMaths16>;
    using op_cmpd_extended =  opcode<op_sub<uint16_t>,   RegisterD_RO,       ExtendedOperand16,   FlagMaths16>;

    using op_cmps_immediate = opcode<op_sub<uint16_t>,   RegisterSP_RO,      ImmediateOperand16,  FlagMaths16>;
    using op_cmps_direct =    opcode<op_sub<uint16_t>,   RegisterSP_RO,      DirectOperand16,     FlagMaths16>;
    using op_cmps_indexed =   opcode<op_sub<uint16_t>,   RegisterSP_RO,      IndexedOperand16,    FlagMaths16>;
    using op_cmps_extended =  opcode<op_sub<uint16_t>,   RegisterSP_RO,      ExtendedOperand16,   FlagMaths16>;

    using op_cmpu_immediate = opcode<op_sub<uint16_t>,   RegisterUSP_RO,     ImmediateOperand16,  FlagMaths16>;
    using op_cmpu_direct =    opcode<op_sub<uint16_t>,   RegisterUSP_RO,     DirectOperand16,     FlagMaths16>;
    using op_cmpu_indexed =   opcode<op_sub<uint16_t>,   RegisterUSP_RO,     IndexedOperand16,    FlagMaths16>;
    using op_cmpu_extended =  opcode<op_sub<uint16_t>,   RegisterUSP_RO,     ExtendedOperand16,   FlagMaths16>;

    using op_cmpx_immediate = opcode<op_sub<uint16_t>,   RegisterX_RO,       ImmediateOperand16,  FlagMaths16>;
    using op_cmpx_direct =    opcode<op_sub<uint16_t>,   RegisterX_RO,       DirectOperand16,     FlagMaths16>;
    using op_cmpx_indexed =   opcode<op_sub<uint16_t>,   RegisterX_RO,       IndexedOperand16,    FlagMaths16>;
    using op_cmpx_extended =  opcode<op_sub<uint16_t>,   RegisterX_RO,       ExtendedOperand16,   FlagMaths16>;

    using op_cmpy_immediate = opcode<op_sub<uint16_t>,   RegisterY_RO,       ImmediateOperand16,  FlagMaths16>;
    using op_cmpy_direct =    opcode<op_sub<uint16_t>,   RegisterY_RO,       DirectOperand16,     FlagMaths16>;
    using op_cmpy_indexed =   opcode<op_sub<uint16_t>,   RegisterY_RO,       IndexedOperand16,    FlagMaths16>;
    using op_cmpy_extended =  opcode<op_sub<uint16_t>,   RegisterY_RO,       ExtendedOperand16,   FlagMaths16>;

    // COM
    using op_coma_inherent = opcode<op_com,            RegisterA,          inherent,           compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>>;
    using op_comb_inherent = opcode<op_com,            RegisterB,          inherent,           compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>>;

    using op_com_direct     = opcode<op_com,            DirectOperand8,     inherent,           compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>>;
    using op_com_indexed    = opcode<op_com,            IndexedOperand8,    inherent,           compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>>;
    using op_com_extended   = opcode<op_com,            ExtendedOperand8,   inherent,           compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>>;

    // CWAI
    using op_cwai_immediate = opcode<op_cwai,           RegisterCC,         ImmediateOperand8>;

    // DAA
    //using op_daa_inherent   = opcode<op_daa,            RegisterA,          inherent,           compute_flags<FLAG_C|FLAG_Z|FLAG_N, 0, FLAG_V>>;

    // DEC
    using op_deca_inherent  = opcode<op_dec,            RegisterA,          inherent,           compute_flags<FLAG_V|FLAG_Z|FLAG_N>>;
    using op_decb_inherent  = opcode<op_dec,            RegisterB,          inherent,           compute_flags<FLAG_V|FLAG_Z|FLAG_N>>;

    using op_dec_direct     = opcode<op_dec,            DirectOperand8,     inherent,           compute_flags<FLAG_V|FLAG_Z|FLAG_N>>;
    using op_dec_indexed    = opcode<op_dec,            IndexedOperand8,    inherent,           compute_flags<FLAG_V|FLAG_Z|FLAG_N>>;
    using op_dec_extended   = opcode<op_dec,            ExtendedOperand8,   inherent,           compute_flags<FLAG_V|FLAG_Z|FLAG_N>>;

    // EOR
    using op_eora_immediate  = opcode<op_eor,           RegisterA,          ImmediateOperand8,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_eora_direct     = opcode<op_eor,           RegisterA,          DirectOperand8,     compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_eora_indexed    = opcode<op_eor,           RegisterA,          IndexedOperand8,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_eora_extended   = opcode<op_eor,           RegisterA,          ExtendedOperand8,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;

    using op_eorb_immediate  = opcode<op_eor,           RegisterB,          ImmediateOperand8,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_eorb_direct     = opcode<op_eor,           RegisterB,          DirectOperand8,     compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_eorb_indexed    = opcode<op_eor,           RegisterB,          IndexedOperand8,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_eorb_extended   = opcode<op_eor,           RegisterB,          ExtendedOperand8,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;

    // EXG
    using op_exg_immediate    = opcode<op_exg,           ImmediateOperand8>;

    // INC
    using op_inca_inherent  = opcode<op_inc,            RegisterA,          inherent,           compute_flags<FLAG_V|FLAG_Z|FLAG_N>>;
    using op_incb_inherent  = opcode<op_inc,            RegisterB,          inherent,           compute_flags<FLAG_V|FLAG_Z|FLAG_N>>;

    using op_inc_direct     = opcode<op_inc,            DirectOperand8,     inherent,           compute_flags<FLAG_V|FLAG_Z|FLAG_N>>;
    using op_inc_indexed    = opcode<op_inc,            IndexedOperand8,    inherent,           compute_flags<FLAG_V|FLAG_Z|FLAG_N>>;
    using op_inc_extended   = opcode<op_inc,            ExtendedOperand8,   inherent,           compute_flags<FLAG_V|FLAG_Z|FLAG_N>>;

    // JMP - copies the EA in to the PC register
    using op_jmp_direct     = opcode<op_copy<uint16_t>, RegisterPC, DirectEA,   NoFlags16>;
    using op_jmp_indexed    = opcode<op_copy<uint16_t>, RegisterPC, IndexedEA,  NoFlags16>;
    using op_jmp_extended   = opcode<op_copy<uint16_t>, RegisterPC, ExtendedEA, NoFlags16>;

    // JSR
    using op_jsr_direct     = opcode<op_jsr, RegisterPC, DirectEA>;
    using op_jsr_indexed    = opcode<op_jsr, RegisterPC, IndexedEA>;
    using op_jsr_extended   = opcode<op_jsr, RegisterPC, ExtendedEA>;

    // LD
    using op_lda_immediate  = opcode<op_copy<uint8_t>,   RegisterA,         ImmediateOperand8,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_lda_direct     = opcode<op_copy<uint8_t>,   RegisterA,         DirectOperand8,     compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_lda_indexed    = opcode<op_copy<uint8_t>,   RegisterA,         IndexedOperand8,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_lda_extended   = opcode<op_copy<uint8_t>,   RegisterA,         ExtendedOperand8,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;

    using op_ldb_immediate  = opcode<op_copy<uint8_t>,   RegisterB,         ImmediateOperand8,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_ldb_direct     = opcode<op_copy<uint8_t>,   RegisterB,         DirectOperand8,     compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_ldb_indexed    = opcode<op_copy<uint8_t>,   RegisterB,         IndexedOperand8,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_ldb_extended   = opcode<op_copy<uint8_t>,   RegisterB,         ExtendedOperand8,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;

    using op_ldd_immediate  = opcode<op_copy<uint16_t>,  RegisterD,         ImmediateOperand16, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldd_direct     = opcode<op_copy<uint16_t>,  RegisterD,         DirectOperand16,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldd_indexed    = opcode<op_copy<uint16_t>,  RegisterD,         IndexedOperand16,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldd_extended   = opcode<op_copy<uint16_t>,  RegisterD,         ExtendedOperand16,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;

    using op_lds_immediate  = opcode<op_copy<uint16_t>,  RegisterSP,        ImmediateOperand16, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_lds_direct     = opcode<op_copy<uint16_t>,  RegisterSP,        DirectOperand16,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_lds_indexed    = opcode<op_copy<uint16_t>,  RegisterSP,        IndexedOperand16,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_lds_extended   = opcode<op_copy<uint16_t>,  RegisterSP,        ExtendedOperand16,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;

    using op_ldu_immediate  = opcode<op_copy<uint16_t>,  RegisterUSP,       ImmediateOperand16, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldu_direct     = opcode<op_copy<uint16_t>,  RegisterUSP,       DirectOperand16,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldu_indexed    = opcode<op_copy<uint16_t>,  RegisterUSP,       IndexedOperand16,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldu_extended   = opcode<op_copy<uint16_t>,  RegisterUSP,       ExtendedOperand16,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;

    using op_ldx_immediate  = opcode<op_copy<uint16_t>,  RegisterX,         ImmediateOperand16, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldx_direct     = opcode<op_copy<uint16_t>,  RegisterX,         DirectOperand16,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldx_indexed    = opcode<op_copy<uint16_t>,  RegisterX,         IndexedOperand16,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldx_extended   = opcode<op_copy<uint16_t>,  RegisterX,         ExtendedOperand16,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;

    using op_ldy_immediate  = opcode<op_copy<uint16_t>,  RegisterY,         ImmediateOperand16, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldy_direct     = opcode<op_copy<uint16_t>,  RegisterY,         DirectOperand16,    compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldy_indexed    = opcode<op_copy<uint16_t>,  RegisterY,         IndexedOperand16,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_ldy_extended   = opcode<op_copy<uint16_t>,  RegisterY,         ExtendedOperand16,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;

    // LEA
    using op_leas_indexed   = opcode<op_copy<uint16_t>,  RegisterSP,        IndexedEA,          NoFlags16>;
    using op_leau_indexed   = opcode<op_copy<uint16_t>,  RegisterUSP,       IndexedEA,          NoFlags16>;
    using op_leax_indexed   = opcode<op_copy<uint16_t>,  RegisterX,         IndexedEA,          compute_flags<FLAG_Z, 0, 0, uint16_t>>;
    using op_leay_indexed   = opcode<op_copy<uint16_t>,  RegisterY,         IndexedEA,          compute_flags<FLAG_Z, 0, 0, uint16_t>>;

    // LSL
    using op_lsla_inherent  = opcode<op_lsl,             RegisterA,          inherent,          FlagMaths>;
    using op_lslb_inherent  = opcode<op_lsl,             RegisterB,          inherent,          FlagMaths>;

    using op_lsl_direct     = opcode<op_lsl,             DirectOperand8,     inherent,          FlagMaths>;
    using op_lsl_indexed    = opcode<op_lsl,             IndexedOperand8,    inherent,          FlagMaths>;
    using op_lsl_extended   = opcode<op_lsl,             ExtendedOperand8,   inherent,          FlagMaths>;

    // LSR
    using op_lsra_inherent  = opcode<op_lsr,             RegisterA,          inherent,          compute_flags<FLAG_Z|FLAG_C, 0, FLAG_N>>;
    using op_lsrb_inherent  = opcode<op_lsr,             RegisterB,          inherent,          compute_flags<FLAG_Z|FLAG_C, 0, FLAG_N>>;

    using op_lsr_direct     = opcode<op_lsr,             DirectOperand8,     inherent,          compute_flags<FLAG_Z|FLAG_C, 0, FLAG_N>>;
    using op_lsr_indexed    = opcode<op_lsr,             IndexedOperand8,    inherent,          compute_flags<FLAG_Z|FLAG_C, 0, FLAG_N>>;
    using op_lsr_extended   = opcode<op_lsr,             ExtendedOperand8,   inherent,          compute_flags<FLAG_Z|FLAG_C, 0, FLAG_N>>;

    // MUL

    // NEG
    using op_nega_inherent  = opcode<op_neg,             RegisterA,          inherent,          FlagMaths>;
    using op_negb_inherent  = opcode<op_neg,             RegisterB,          inherent,          FlagMaths>;

    using op_neg_direct     = opcode<op_neg,             DirectOperand8,     inherent,          FlagMaths>;
    using op_neg_indexed    = opcode<op_neg,             IndexedOperand8,    inherent,          FlagMaths>;
    using op_neg_extended   = opcode<op_neg,             ExtendedOperand8,   inherent,          FlagMaths>;

    // NOP
    using op_nop_inherent   = opcode<op_nop>;

    // OR
    using op_ora_immediate  = opcode<op_or,             RegisterA,          ImmediateOperand8,  LogicFlags>;
    using op_ora_direct     = opcode<op_or,             DirectOperand8,     RegisterA,          LogicFlags>;
    using op_ora_indexed    = opcode<op_or,             IndexedOperand8,    RegisterA,          LogicFlags>;
    using op_ora_extended   = opcode<op_or,             ExtendedOperand8,   RegisterA,          LogicFlags>;

    using op_orb_immediate  = opcode<op_or,             RegisterB,          ImmediateOperand8,  LogicFlags>;
    using op_orb_direct     = opcode<op_or,             DirectOperand8,     RegisterB,          LogicFlags>;
    using op_orb_indexed    = opcode<op_or,             IndexedOperand8,    RegisterB,          LogicFlags>;
    using op_orb_extended   = opcode<op_or,             ExtendedOperand8,   RegisterB,          LogicFlags>;

    // ORCC - do not update the flags, they are affected by the immediate value
    using op_orcc_immediate = opcode<op_or,             RegisterCC,         ImmediateOperand8>;

    // PSHS / PSHU
    using op_pshs_immediate  = opcode<op_push<reg_sp>,  ImmediateOperand8>;
    using op_pshu_immediate  = opcode<op_push<reg_usp>, ImmediateOperand8>;

    // PULS / PULU
    using op_puls_immediate  = opcode<op_pull<reg_sp>,  ImmediateOperand8>;
    using op_pulu_immediate  = opcode<op_pull<reg_usp>, ImmediateOperand8>;

    // ROL
    using op_rola_inherent  = opcode<op_rol,             RegisterA,          inherent,          FlagMaths>;
    using op_rolb_inherent  = opcode<op_rol,             RegisterB,          inherent,          FlagMaths>;

    using op_rol_direct     = opcode<op_rol,             DirectOperand8,     inherent,          FlagMaths>;
    using op_rol_indexed    = opcode<op_rol,             IndexedOperand8,    inherent,          FlagMaths>;
    using op_rol_extended   = opcode<op_rol,             ExtendedOperand8,   inherent,          FlagMaths>;

    // ROR
    using op_rora_inherent  = opcode<op_ror,             RegisterA,          inherent,          FlagMaths>;
    using op_rorb_inherent  = opcode<op_ror,             RegisterB,          inherent,          FlagMaths>;

    using op_ror_direct     = opcode<op_ror,             DirectOperand8,     inherent,          FlagMaths>;
    using op_ror_indexed    = opcode<op_ror,             IndexedOperand8,    inherent,          FlagMaths>;
    using op_ror_extended   = opcode<op_ror,             ExtendedOperand8,   inherent,          FlagMaths>;

    // RTI
    using op_rti_inherent   = opcode<op_rti, RegisterPC, inherent, NoFlags16>;

    // RTS
    using op_rts_inherent   = opcode<op_rts, RegisterPC, inherent, NoFlags16>;

    // SBC - subtract with carry
    using op_sbca_immediate = opcode<op_sbc,            RegisterA,          ImmediateOperand8,  FlagMaths>;
    using op_sbca_direct    = opcode<op_sbc,            DirectOperand8,     RegisterA,          FlagMaths>;
    using op_sbca_indexed   = opcode<op_sbc,            IndexedOperand8,    RegisterA,          FlagMaths>;
    using op_sbca_extended  = opcode<op_sbc,            ExtendedOperand8,   RegisterA,          FlagMaths>;

    using op_sbcb_immediate = opcode<op_sbc,            RegisterB,          ImmediateOperand8,  FlagMaths>;
    using op_sbcb_direct    = opcode<op_sbc,            DirectOperand8,     RegisterB,          FlagMaths>;
    using op_sbcb_indexed   = opcode<op_sbc,            IndexedOperand8,    RegisterB,          FlagMaths>;
    using op_sbcb_extended  = opcode<op_sbc,            ExtendedOperand8,   RegisterB,          FlagMaths>;

    // SEX sign extend A in to B
    using op_sex_inherent   = opcode<op_sex,            RegisterB,          RegisterA,          compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>>;

    // ST (reverse of LD)
    using op_sta_immediate  = opcode<op_copy<uint8_t>,   ImmediateOperand8,  RegisterA,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_sta_direct     = opcode<op_copy<uint8_t>,   DirectOperand8,     RegisterA,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_sta_indexed    = opcode<op_copy<uint8_t>,   IndexedOperand8,    RegisterA,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_sta_extended   = opcode<op_copy<uint8_t>,   ExtendedOperand8,   RegisterA,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;

    using op_stb_immediate  = opcode<op_copy<uint8_t>,   ImmediateOperand8,  RegisterB,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_stb_direct     = opcode<op_copy<uint8_t>,   DirectOperand8,     RegisterB,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_stb_indexed    = opcode<op_copy<uint8_t>,   IndexedOperand8,    RegisterB,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;
    using op_stb_extended   = opcode<op_copy<uint8_t>,   ExtendedOperand8,   RegisterB,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V>>;

    using op_std_immediate  = opcode<op_copy<uint16_t>,  ImmediateOperand16, RegisterD,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_std_direct     = opcode<op_copy<uint16_t>,  DirectOperand16,    RegisterD,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_std_indexed    = opcode<op_copy<uint16_t>,  IndexedOperand16,   RegisterD,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_std_extended   = opcode<op_copy<uint16_t>,  ExtendedOperand16,  RegisterD,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;

    using op_sts_immediate  = opcode<op_copy<uint16_t>,  ImmediateOperand16, RegisterSP,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_sts_direct     = opcode<op_copy<uint16_t>,  DirectOperand16,    RegisterSP,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_sts_indexed    = opcode<op_copy<uint16_t>,  IndexedOperand16,   RegisterSP,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_sts_extended   = opcode<op_copy<uint16_t>,  ExtendedOperand16,  RegisterSP,  compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;

    using op_stu_immediate  = opcode<op_copy<uint16_t>,  ImmediateOperand16, RegisterUSP, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_stu_direct     = opcode<op_copy<uint16_t>,  DirectOperand16,    RegisterUSP, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_stu_indexed    = opcode<op_copy<uint16_t>,  IndexedOperand16,   RegisterUSP, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_stu_extended   = opcode<op_copy<uint16_t>,  ExtendedOperand16,  RegisterUSP, compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;

    using op_stx_immediate  = opcode<op_copy<uint16_t>,  ImmediateOperand16, RegisterX,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_stx_direct     = opcode<op_copy<uint16_t>,  DirectOperand16,    RegisterX,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_stx_indexed    = opcode<op_copy<uint16_t>,  IndexedOperand16,   RegisterX,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_stx_extended   = opcode<op_copy<uint16_t>,  ExtendedOperand16,  RegisterX,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;

    using op_sty_immediate  = opcode<op_copy<uint16_t>,  ImmediateOperand16, RegisterY,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_sty_direct     = opcode<op_copy<uint16_t>,  DirectOperand16,    RegisterY,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_sty_indexed    = opcode<op_copy<uint16_t>,  IndexedOperand16,   RegisterY,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;
    using op_sty_extended   = opcode<op_copy<uint16_t>,  ExtendedOperand16,  RegisterY,   compute_flags<FLAG_Z|FLAG_N, 0, FLAG_V, uint16_t>>;

    // SUB
    using op_suba_immediate = opcode<op_sub<uint8_t>,  RegisterA,         ImmediateOperand8,  FlagMaths>;
    using op_suba_direct    = opcode<op_sub<uint8_t>,  DirectOperand8,    RegisterA,          FlagMaths>;
    using op_suba_indexed   = opcode<op_sub<uint8_t>,  IndexedOperand8,   RegisterA,          FlagMaths>;
    using op_suba_extended  = opcode<op_sub<uint8_t>,  ExtendedOperand8,  RegisterA,          FlagMaths>;

    using op_subb_immediate = opcode<op_sub<uint8_t>,  RegisterB,         ImmediateOperand8,  FlagMaths>;
    using op_subb_direct    = opcode<op_sub<uint8_t>,  DirectOperand8,    RegisterB,          FlagMaths>;
    using op_subb_indexed   = opcode<op_sub<uint8_t>,  IndexedOperand8,   RegisterB,          FlagMaths>;
    using op_subb_extended  = opcode<op_sub<uint8_t>,  ExtendedOperand8,  RegisterB,          FlagMaths>;

    using op_subd_immediate = opcode<op_sub<uint16_t>, RegisterD,         ImmediateOperand16, FlagMaths16>;
    using op_subd_direct    = opcode<op_sub<uint16_t>, DirectOperand16,   RegisterD,          FlagMaths16>;
    using op_subd_indexed   = opcode<op_sub<uint16_t>, IndexedOperand16,  RegisterD,          FlagMaths16>;
    using op_subd_extended  = opcode<op_sub<uint16_t>, ExtendedOperand16, RegisterD,          FlagMaths16>;

    // SWI

    // SYNC

    // TFR
    using op_tfr_immediate   = opcode<op_tfr, ImmediateOperand8>;

    // TST - test
    using op_tsta_inherent   = opcode<op_tst, RegisterA_RO,                        inherent, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>>;
    using op_tstb_inherent   = opcode<op_tst, RegisterB_RO,                        inherent, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>>;

    // use read-only memory operands
    using op_tst_direct      = opcode<op_tst, MemoryOperand<uint8_t, direct, 0>,   inherent, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>>;
    using op_tst_indexed     = opcode<op_tst, MemoryOperand<uint8_t, indexed, 0>,  inherent, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>>;
    using op_tst_extended    = opcode<op_tst, MemoryOperand<uint8_t, extended, 0>, inherent, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>>;


public:
    M6809();

    // Reset the CPU to it's default state, clearing the registers and setting the PC to the reset vector
    void Reset();

    // Set callbacks for read and write, must be a static function
    void SetReadCallback(read_callback_t func, intptr_t ref);
    void SetWriteCallback(write_callback_t func, intptr_t ref);

    // Exceture one instruction and updated the number of cycles that it took
    m6809_error_t Execute(int &cycles);

    // flag computation
    template<typename T>
    static inline void ComputeZeroFlag(uint8_t &flags, const T &value)
    {
        flags &= ~FLAG_Z;
        flags |= FLAG_Z * ((value == 0) ? 1u : 0u);
    };

    template <typename T>
    static inline void ComputeNegativeFlag(uint8_t &flags, const T &value)
    {
        flags &= ~FLAG_N;
        flags |= FLAG_N * (value >> ((sizeof(T) * 8) - 1u) & 1u);
    };

    template <typename T>
    static inline void ComputeCarryFlag(uint8_t &flags, const T &opa, const T &opb, const T &result)
    {
        uint16_t flag  = (opa | opb) & ~result;  // one of the inputs is 1 and output is 0
        flag |= (opa & opb);                     // both inputs are 1
        flags &= ~FLAG_C;
        flags |= FLAG_C * (flag >> ((sizeof(T) * 8) - 1u) & 1u);
    };

    static inline void ComputeHalfCarryFlag(uint8_t &flags, const uint8_t &opa, const uint8_t &opb, const uint8_t &result)
    {
        uint16_t flag  = (opa | opb) & ~result;  // one of the inputs is 1 and output is 0
        flag |= (opa & opb);                     // both inputs are 1
        flags |= FLAG_H * (flag >> 3 & 1u);
    };

    template <typename T>
    static inline void ComputeOverflowFlag(uint8_t &flags, const T &opa, const T &opb, const T &result)
    {
        // if the sign bit is the same in both operands but
        // different in the result, then there has been an overflow
        auto bits = sizeof(T) * 8;
        auto set = ((opa >> (bits - 1u)) == (opb >> (bits - 1u)) && (opb >> (bits - 1u)) != (result >> (bits - 1u))) ? 1u : 0u;
        flags &= ~FLAG_V;
        flags |= FLAG_H * set;
    }

    M6809Registers &getRegisters() { return registers; }

};


#endif //VECTREXIA_M6809_H
