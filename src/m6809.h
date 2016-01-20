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
    using opcode_handler_t = void(*)(M6809&, int&);
    using read_callback_t = std::function<uint8_t(uint16_t)>;
    using write_callback_t = std::function<void(uint16_t, uint8_t)>;

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

    uint16_t *index_mode_register_table[4] = {
            &registers.X,
            &registers.Y,
            &registers.USP,
            &registers.SP
    };

    // memory accessors
    //   callbacks
    read_callback_t read_callback = {};
    write_callback_t write_callback = {};

    uint8_t Read8(uint16_t addr);
    uint16_t Read16(uint16_t addr);

    void Write8(uint16_t addr, uint8_t data);
    void Write16(uint16_t addr, uint16_t data);

    // read 8/16 bits from relative to the pc
    uint8_t NextOpcode();
    uint8_t ReadPC8();
    uint16_t ReadPC16();

    /*
     * OpCode Templates
     */
    struct reg_a { uint8_t &operator() (M6809& cpu, uint16_t &a) { return cpu.registers.A; } };
    struct reg_b { uint8_t &operator() (M6809& cpu, uint16_t &a) { return cpu.registers.B; } };
    struct reg_d { uint16_t &operator() (M6809& cpu, uint16_t &a) { return cpu.registers.D; } };
    struct reg_x { uint16_t &operator() (M6809& cpu, uint16_t &a) { return cpu.registers.X; } };
    struct reg_y { uint16_t &operator() (M6809& cpu, uint16_t &a) { return cpu.registers.Y; } };
    struct reg_cc { uint8_t &operator() (M6809& cpu, uint16_t &a) { return cpu.registers.CC; } };

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
                        reg += 1 + (post_byte & 1);
                        break;
                    case 2:
                    case 3:
                        // ,-R | ,--R
                        ea = reg;
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
                        throw E_ILLEGAL_INDEXING_MODE;
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
     * Opcode implementations
     */
    template <typename T>
    struct op_add { T operator() (M6809& cpu, T operand_a, T operand_b) { return operand_a + operand_b; } };

    template <typename T>
    struct op_sub {
        T operator() (M6809& cpu, T operand_a, T operand_b) {
            return operand_a - operand_b;
        }
    };

    struct op_and { uint8_t operator() (M6809& cpu, uint8_t operand_a, uint8_t operand_b) { return operand_a & operand_b; } };
    // store/load, M <= Register or Register <= Memory
    struct op_copy { uint16_t operator() (M6809& cpu, uint16_t operand_a, uint16_t operand_b) { return operand_b; } };

    // one operand
    struct op_dec { uint16_t operator() (M6809& cpu, uint16_t operand) { return (uint16_t) (operand - 1); } };
    struct op_clr { uint16_t operator() (M6809& cpu, uint16_t operand) { return 0; } };

    // no operands
    struct op_nop { uint16_t operator() (M6809& cpu) { return 0u; } };


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

        T operator ()(uint16_t &addr) {
            addr = Fn()(cpu);
            return (std::is_same<T, uint8_t>::value) ? cpu.Read8(addr) : cpu.Read16(addr);
        }
        void update(uint16_t addr, T data) {
            if (RW)
            {
                if (std::is_same<T, uint8_t>::value)
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

        T operator ()(uint16_t &addr) {
            if (std::is_same<T, uint8_t>::value)
                return cpu.ReadPC8();
            else
                return cpu.ReadPC16();
        }
        void update(uint16_t addr, T data) { }
    };

    template <typename T, typename Fn, int RW=1>
    struct Register : Operand
    {
        Register(M6809 &cpu) : Operand(cpu) { }

        uint8_t operator() (uint16_t &addr) {
            return Fn()(cpu, addr);
        }
        void update(uint16_t addr, T data) {
            if (RW)
            {
                Fn()(cpu, addr) = data;
            }
        }
    };

    /*
     * Calculate flags
     */
    template <int FlagUpdateMask=0, int FlagSetMask=0, int FlagClearMask=0, int BitSize=8>
    struct compute_flags
    {
        void operator() (M6809 &cpu, uint16_t result, uint16_t operand_a, uint16_t operand_b)
        {
            cpu.registers.CC &= ~FlagClearMask;
            cpu.registers.CC |= FlagSetMask;

            if (FlagUpdateMask & FLAG_Z) cpu.registers.flags.Z = cpu.ComputeZeroFlag<BitSize>(result);
            if (FlagUpdateMask & FLAG_N) cpu.registers.flags.N = cpu.ComputeNegativeFlag(result);
            if (FlagUpdateMask & FLAG_H) cpu.registers.flags.H = cpu.ComputeCarryFlag<4>(operand_a, operand_b, result);
            if (FlagUpdateMask & FLAG_V) cpu.registers.flags.V = cpu.ComputeOverflowFlag<BitSize>(operand_a, operand_b, result);
            if (FlagUpdateMask & FLAG_C) cpu.registers.flags.C = cpu.ComputeCarryFlag<BitSize>(operand_a, operand_b, result);
        }
    };

    /*
     * Opcode Templates
     */
    template<typename Fn, typename OpA=inherent, typename OpB=inherent, typename Flags=compute_flags<>, int W=1, int BitSize=8>
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

    template<typename Fn, typename OpA, typename Flags, int W, int BitSize>
    struct opcode<Fn, OpA, inherent, Flags, W, BitSize>
    {
        void operator() (M6809& cpu, int &cycles)
        {
            uint16_t operand_addr = 0;
            auto operand = OpA(cpu);
            auto operand_value = operand(operand_addr);
            auto result = Fn()(cpu, operand_value);

            Flags()(cpu, result, 0, operand_value);

            operand.update(operand_addr, result);
        }
    };

    template<typename Fn, typename Flags, int W, int BitSize>
    struct opcode<Fn, inherent, inherent, Flags, W, BitSize>
    {
        void operator() (M6809& cpu, int &cycles)
        {
            auto result = Fn()(cpu);
            Flags()(cpu, result, 0, 0);
        }
    };

    template<typename Op>
    static void opcodewrap (M6809& cpu, int &cycles)
    {
        Op()(cpu, cycles);
    }

    /*
     * Alaises for Registers and memory addressing modes
     */
    using RegisterA = Register<uint8_t, reg_a>;
    using RegisterB = Register<uint8_t, reg_b>;
    using RegisterA_RO = Register<uint8_t, reg_a, 0>;  // read-only version
    using RegisterB_RO = Register<uint8_t, reg_b, 0>;  // read-only version
    using RegisterD = Register<uint16_t, reg_d>;
    using RegisterX = Register<uint16_t, reg_x>;
    using RegisterY = Register<uint16_t, reg_y>;
    using RegisterCC = Register<uint8_t, reg_cc>;

    using ImmediateOperand8 = MemoryOperand<uint8_t, immediate>;
    using ImmediateOperand16 = MemoryOperand<uint16_t, immediate>;
    using DirectOperand8 = MemoryOperand<uint8_t, direct>;
    using DirectOperand16 = MemoryOperand<uint16_t, direct>;
    using ExtendedOperand8 = MemoryOperand<uint8_t, extended>;
    using ExtendedOperand16 = MemoryOperand<uint16_t, extended>;
    using IndexedOperand8 = MemoryOperand<uint8_t, indexed>;
    using IndexedOperand16 = MemoryOperand<uint16_t, indexed>;

    using FlagMaths = compute_flags<FLAGS_MATH>;
    using FlagMaths16 = compute_flags<FLAGS_MATH, 0, 0, 16>;
    /*
     * Opcode handler definitions
     */
    std::array<opcode_handler_t, 0x100> opcode_handlers;

    using op_abx_inherent = opcode<op_add<uint16_t>, RegisterX, RegisterB>;

    using op_adda_immediate = opcode<op_add<uint8_t>, RegisterA, ImmediateOperand8, FlagMaths>;
    using op_adda_direct = opcode<op_add<uint8_t>, DirectOperand8, RegisterA, FlagMaths>;
    using op_adda_extended = opcode<op_add<uint8_t>, ExtendedOperand8, RegisterA, FlagMaths>;

    using op_addb_immediate = opcode<op_add<uint8_t>, RegisterB, ImmediateOperand8, FlagMaths>;
    using op_addb_direct = opcode<op_add<uint8_t>, DirectOperand8, RegisterB, FlagMaths>;
    using op_addb_extended = opcode<op_add<uint8_t>, ExtendedOperand8, RegisterB, FlagMaths>;

    using op_addd_immediate = opcode<op_add<uint16_t>, RegisterD, ImmediateOperand16, FlagMaths16>;

    // BIT - bitwise AND, update flags but don't store the result
    using op_bita_immediate = opcode<op_and, RegisterA_RO, ImmediateOperand8, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>>;
    using op_bitb_immediate = opcode<op_and, RegisterB_RO, ImmediateOperand8, compute_flags<FLAG_N|FLAG_Z, 0, FLAG_V>>;

    using op_suba_immediate = opcode<op_sub<int8_t>, RegisterA, ImmediateOperand8, FlagMaths>;

    using op_dec_direct = opcode<op_dec, DirectOperand8, inherent, FlagMaths>;
    using op_dec_extended = opcode<op_dec, ExtendedOperand8, inherent, FlagMaths>;

    //opcode<op_nop> op_nop_inherent;

    //auto op_adda_inherent = opcode2<op_add<reg_a, inherent>, write_memory8, compute_flags<FLAG_Z|FLAG_N|FLAG_C|FLAG_V>>;

    //auto op_adda_direct = opcode2<op_add<reg_a, direct>, direct, compute_flags<FLAG_Z|FLAG_N|FLAG_C|FLAG_V>>;



public:
    M6809(const read_callback_t &read_callback,
          const write_callback_t &write_callback);

    // Reset the CPU to it's default state, clearing the registers and setting the PC to the reset vector
    void Reset();

    // Exceture one instruction and updated the number of cycles that it took
    m6809_error_t Execute(int &cycles);

    // flag computation
    template <unsigned bits=8>
    static uint8_t ComputeZeroFlag(uint16_t value)
    {
        return (uint8_t)(((value & (bits == 8 ? 0xff : 0xffff)) == 0) ? 1u : 0u);
    };

    template <unsigned bits=8>
    static uint8_t ComputeNegativeFlag(uint16_t value)
    {
        return (uint8_t) (value >> (bits - 1u) & 1u);
    };

    template <unsigned bits=8>
    static uint8_t ComputeCarryFlag(uint16_t opa, uint16_t opb, uint16_t result)
    {
        uint16_t flag  = (opa | opb) & ~result;  // one of the inputs is 1 and output is 0
        flag |= (opa & opb);                     // both inputs are 1
        return (uint8_t) (flag >> (bits - 1u) & 1u);
    };

    template <unsigned bits=8>
    static uint8_t ComputeOverflowFlag(uint16_t opa, uint16_t opb, uint16_t result)
    {
        // if the sign bit is the same in both operands but
        // different in the result, then there has been an overflow
        return (uint8_t) ((opa >> (bits - 1u)) == (opb >> (bits - 1u)) &&
                                  (opb >> (bits - 1u)) != (result >> (bits - 1u)) ? 1u : 0u);
    }

    M6809Registers &getRegisters() { return registers; }

};


#endif //VECTREXIA_M6809_H
