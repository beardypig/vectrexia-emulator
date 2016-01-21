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
    struct reg_sp { uint16_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.SP; } };
    struct reg_usp { uint16_t &operator() (M6809& cpu, const uint16_t &a) { return cpu.registers.USP; } };

    /*
     * Memory addressing implementations
     */
    struct immediate { uint16_t operator()(M6809& cpu) { return cpu.registers.PC++; } };
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
            return (sizeof(T) == 8) ? cpu.Read8(addr) : cpu.Read16(addr);
        }
        void update(uint16_t addr, T data) {
            if (RW)
            {
                if (sizeof(T) == 8)
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
            if (sizeof(T) == 8)
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

    /*
     * Alaises for Registers and memory addressing modes
     */
    using RegisterA = Register<uint8_t, reg_a>;
    using RegisterB = Register<uint8_t, reg_b>;
    using RegisterD = Register<uint16_t, reg_d>;
    using RegisterX = Register<uint16_t, reg_x>;
    using RegisterY = Register<uint16_t, reg_y>;
    using RegisterCC = Register<uint8_t, reg_cc>;
    // read-only version
    using RegisterA_RO    = Register<uint8_t,  reg_a,   0>;
    using RegisterB_RO    = Register<uint8_t,  reg_b,   0>;
    using RegisterD_RO    = Register<uint16_t, reg_d,   0>;
    using RegisterX_RO    = Register<uint16_t, reg_x,   0>;
    using RegisterY_RO    = Register<uint16_t, reg_y,   0>;
    using RegisterCC_RO   = Register<uint8_t,  reg_cc,  0>;
    using RegisterSP_RO   = Register<uint8_t,  reg_sp,  0>;
    using RegisterUSP_RO  = Register<uint8_t,  reg_usp, 0>;

    using ImmediateOperand8 = MemoryOperand<uint8_t, immediate>;
    using ImmediateOperand16 = MemoryOperand<uint16_t, immediate>;
    using DirectOperand8 = MemoryOperand<uint8_t, direct>;
    using DirectOperand16 = MemoryOperand<uint16_t, direct>;
    using ExtendedOperand8 = MemoryOperand<uint8_t, extended>;
    using ExtendedOperand16 = MemoryOperand<uint16_t, extended>;
    using IndexedOperand8 = MemoryOperand<uint8_t, indexed>;
    using IndexedOperand16 = MemoryOperand<uint16_t, indexed>;

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

    /*
     * Opcode implementations
     */
    template <typename T1, typename T2=T1>
    struct op_add { T1 operator() (M6809& cpu, const T1 &operand_a, const T2 &operand_b) { return operand_a + operand_b; } };

    struct op_adc { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b) { return operand_a + operand_b + cpu.registers.flags.C; } };

    template <typename T>
    struct op_sub {
        T operator() (const M6809& cpu, const T &operand_a, const T &operand_b) {
            return operand_a - operand_b;
        }
    };

    struct op_and { uint8_t operator() (const M6809& cpu, const uint8_t &operand_a, const uint8_t &operand_b) { return operand_a & operand_b; } };
    // store/load, M <= Register or Register <= Memory
    struct op_copy { uint16_t operator() (const M6809& cpu, const uint16_t &operand_a, const uint16_t &operand_b) { return operand_b; } };

    // This is a special case where the operation sets a pseudo flag to tell the cpu to wait for an interrupt
    struct op_cwai {
        uint16_t operator() (M6809& cpu, const uint16_t &operand_a, const uint16_t &operand_b) {
            cpu.wait_for_intterupt = true;
            return operand_a ^ operand_b;
        }
    };


    // one operand
    struct op_dec { uint16_t operator() (const M6809& cpu, const uint16_t &operand) { return (uint16_t) (operand - 1); } };
    struct op_clr { uint16_t operator() (const M6809& cpu, const uint16_t &operand) { return 0; } };
    struct op_asr { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return operand >> 1; } };
    struct op_com { uint8_t operator() (const M6809& cpu, const uint8_t &operand) { return ~operand; } };


    // no operands
    struct op_nop { uint16_t operator() (const M6809& cpu) { return 0u; } };

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
    using op_bita_immediate = opcode<op_and,            RegisterA_RO,       ImmediateOperand8,  LogicFlags>;
    using op_bitb_immediate = opcode<op_and,            RegisterB_RO,       ImmediateOperand8,  LogicFlags>;

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

    using op_cmpb_immediate = opcode<op_sub<uint8_t>,   RegisterB_RO,       ImmediateOperand8,  FlagMaths16>;
    using op_cmpb_direct =    opcode<op_sub<uint8_t>,   RegisterB_RO,       DirectOperand8,     FlagMaths16>;
    using op_cmpb_indexed =   opcode<op_sub<uint8_t>,   RegisterB_RO,       IndexedOperand8,    FlagMaths16>;
    using op_cmpb_extended =  opcode<op_sub<uint8_t>,   RegisterB_RO,       ExtendedOperand8,   FlagMaths16>;

    using op_cmpd_immediate = opcode<op_sub<uint8_t>,   RegisterD_RO,       ImmediateOperand8,  FlagMaths16>;
    using op_cmpd_direct =    opcode<op_sub<uint8_t>,   RegisterD_RO,       DirectOperand8,     FlagMaths16>;
    using op_cmpd_indexed =   opcode<op_sub<uint8_t>,   RegisterD_RO,       IndexedOperand8,    FlagMaths16>;
    using op_cmpd_extended =  opcode<op_sub<uint8_t>,   RegisterD_RO,       ExtendedOperand8,   FlagMaths16>;

    using op_cmps_immediate = opcode<op_sub<uint8_t>,   RegisterSP_RO,      ImmediateOperand8,  FlagMaths16>;
    using op_cmps_direct =    opcode<op_sub<uint8_t>,   RegisterSP_RO,      DirectOperand8,     FlagMaths16>;
    using op_cmps_indexed =   opcode<op_sub<uint8_t>,   RegisterSP_RO,      IndexedOperand8,    FlagMaths16>;
    using op_cmps_extended =  opcode<op_sub<uint8_t>,   RegisterSP_RO,      ExtendedOperand8,   FlagMaths16>;

    using op_cmpu_immediate = opcode<op_sub<uint8_t>,   RegisterUSP_RO,     ImmediateOperand8,  FlagMaths16>;
    using op_cmpu_direct =    opcode<op_sub<uint8_t>,   RegisterUSP_RO,     DirectOperand8,     FlagMaths16>;
    using op_cmpu_indexed =   opcode<op_sub<uint8_t>,   RegisterUSP_RO,     IndexedOperand8,    FlagMaths16>;
    using op_cmpu_extended =  opcode<op_sub<uint8_t>,   RegisterUSP_RO,     ExtendedOperand8,   FlagMaths16>;

    using op_cmpx_immediate = opcode<op_sub<uint8_t>,   RegisterX_RO,       ImmediateOperand8,  FlagMaths16>;
    using op_cmpx_direct =    opcode<op_sub<uint8_t>,   RegisterX_RO,       DirectOperand8,     FlagMaths16>;
    using op_cmpx_indexed =   opcode<op_sub<uint8_t>,   RegisterX_RO,       IndexedOperand8,    FlagMaths16>;
    using op_cmpx_extended =  opcode<op_sub<uint8_t>,   RegisterX_RO,       ExtendedOperand8,   FlagMaths16>;

    using op_cmpy_immediate = opcode<op_sub<uint8_t>,   RegisterY_RO,       ImmediateOperand8,  FlagMaths16>;
    using op_cmpy_direct =    opcode<op_sub<uint8_t>,   RegisterY_RO,       DirectOperand8,     FlagMaths16>;
    using op_cmpy_indexed =   opcode<op_sub<uint8_t>,   RegisterY_RO,       IndexedOperand8,    FlagMaths16>;
    using op_cmpy_extended =  opcode<op_sub<uint8_t>,   RegisterY_RO,       ExtendedOperand8,   FlagMaths16>;

    // COM
    using op_coma_immediate = opcode<op_com,            RegisterA,          inherent,           compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>>;
    using op_comb_immediate = opcode<op_com,            RegisterB,          inherent,           compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>>;

    using op_com_direct     = opcode<op_com,            DirectOperand8,     inherent,           compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>>;
    using op_com_indexed    = opcode<op_com,            IndexedOperand8,    inherent,           compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>>;
    using op_com_extended   = opcode<op_com,            ExtendedOperand8,   inherent,           compute_flags<FLAG_N|FLAG_Z, FLAG_C, FLAG_V>>;

    // CWAI
    using op_cawi_immediate = opcode<op_cwai,           RegisterCC,         ImmediateOperand8>;

    using op_suba_immediate = opcode<op_sub<int8_t>, RegisterA, ImmediateOperand8, FlagMaths>;

    using op_dec_direct = opcode<op_dec, DirectOperand8, inherent, FlagMaths>;
    using op_dec_extended = opcode<op_dec, ExtendedOperand8, inherent, FlagMaths>;

    //opcode<op_nop> op_nop_inherent;

    //auto op_adda_inherent = opcode2<op_add<reg_a, inherent>, write_memory8, compute_flags<FLAG_Z|FLAG_N|FLAG_C|FLAG_V>>;

    //auto op_adda_direct = opcode2<op_add<reg_a, direct>, direct, compute_flags<FLAG_Z|FLAG_N|FLAG_C|FLAG_V>>;



public:
    M6809();

    // Reset the CPU to it's default state, clearing the registers and setting the PC to the reset vector
    void Reset();

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
