/*
Copyright (C) 2016 beardypig

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
#ifndef VECTREXIA_M6809_DISASSEMBLE_H
#define VECTREXIA_M6809_DISASSEMBLE_H


#include <string>
#include <sstream>
#include <cstdint>
#include <functional>
#include <sstream>
#include <array>
#include <vector>
#include "veclib.h"

// Enum to represent the type of operand
enum class M6809OperandType {
    REL,
    INHERENT,
    IMMEDIATE,
    INDEXED,
    EXTENDED,
    DIRECT,
    REG
};

// Enum to represent the available registers
enum class M6809Register {
    A,
    B,
    D,
    X,
    Y,
    U,
    S,
    PC,            
    CC,
    DP,
    INVALID
};

inline std::string toString(M6809Register reg) {
    switch (reg) {
    case M6809Register::A: return "A";
    case M6809Register::B: return "B";
    case M6809Register::D: return "D";
    case M6809Register::X: return "X";
    case M6809Register::Y: return "Y";
    case M6809Register::U: return "U";
    case M6809Register::S: return "S";
    case M6809Register::PC: return "PC";
    case M6809Register::CC: return "CC";
    case M6809Register::DP: return "DP";
    default: return "INVALID";
    }
}

struct M6809Operand {
    M6809OperandType type;

    union {
        uint16_t immediate;         // Immediate value
        uint16_t directAddress;     // Direct addressing (0x00-0xFF range)
        uint16_t extendedAddress;   // Extended addressing (16-bit address)

        struct {
            int16_t offset;              // 5-bit or 8-bit constant offset
            bool indirect;              // Indicates if this is indirect (`[n,X]`)
            int8_t increment;           // Post-increment or pre-decrement value (+1, +2, -1, -2)
            M6809Register indexReg;     // Index register used (`X`, `Y`, `U`, `S`)
            M6809Register offsetReg;    // Register offset (e.g., `A`, `B`, `D`), or `INVALID` if none
        } indexed;

        struct {
            uint16_t base;   // Base address (used for relative jumps)
            int16_t offset;         // Offset relative to base address
        } relativeAddress;

        M6809Register reg;          // Register-based operand (for inherent instructions, like `PSHS`)

    };

    // Constructor to initialize the operand type
    M6809Operand(M6809OperandType type) : type(type), indexed({ 0 }) {}
};


struct M6809Instruction
{
    uint16_t address;
    std::string mnemonic;
    std::vector<M6809Operand> operands;
    uint8_t length;

    M6809Instruction(uint16_t address) : address(address), length(1), mnemonic("") {}
    M6809Instruction() : address(0), length(1), mnemonic("") {}
    M6809Instruction(uint16_t address, std::string mnemonic, std::vector<M6809Operand> operands)
        : address(address), mnemonic(std::move(mnemonic)), operands(std::move(operands)), length(0) {
    }
};


class M6809Disassemble
{
    using read_callback_t = uint8_t (*)(intptr_t, uint16_t);
    using disasm_handler_t = M6809Instruction (*)(M6809Disassemble &, uint16_t &);

    // read callback
    read_callback_t read_callback_func;
    intptr_t read_callback_ref;

    std::array<disasm_handler_t, 0x100> disasm_handlers;
    std::array<disasm_handler_t, 0x100> disasm_handlers_page1;
    std::array<disasm_handler_t, 0x100> disasm_handlers_page2;
    const M6809Register index_mode_register_table[4] = {
        M6809Register::X,
        M6809Register::Y,
        M6809Register::U,
        M6809Register::S
    };
    const M6809Register exg_register_table[0xc] = { 
        M6809Register::D, 
        M6809Register::X, 
        M6809Register::Y, 
        M6809Register::U, 
        M6809Register::S, 
        M6809Register::PC, 
        M6809Register::INVALID,
        M6809Register::S, 
        M6809Register::A, 
        M6809Register::B, 
        M6809Register::CC, 
        M6809Register::DP
    };

    struct op_abx { std::string operator()() { return "abx"; } };
    struct op_adca { std::string operator()() { return "adca"; } };
    struct op_adcb { std::string operator()() { return "adcb"; } };
    struct op_adda { std::string operator()() { return "adda"; } };
    struct op_addb { std::string operator()() { return "addb"; } };
    struct op_addd { std::string operator()() { return "addd"; } };
    struct op_anda { std::string operator()() { return "anda"; } };
    struct op_andb { std::string operator()() { return "andb"; } };
    struct op_andcc { std::string operator()() { return "andcc"; } };
    struct op_asr { std::string operator()() { return "asr"; } };
    struct op_asra { std::string operator()() { return "asra"; } };
    struct op_asrb { std::string operator()() { return "asrb"; } };
    struct op_bcc { std::string operator()() { return "bcc"; } };
    struct op_bcs { std::string operator()() { return "bcs"; } };
    struct op_beq { std::string operator()() { return "beq"; } };
    struct op_bge { std::string operator()() { return "bge"; } };
    struct op_bgt { std::string operator()() { return "bgt"; } };
    struct op_bhi { std::string operator()() { return "bhi"; } };
    struct op_bita { std::string operator()() { return "bita"; } };
    struct op_bitb { std::string operator()() { return "bitb"; } };
    struct op_ble { std::string operator()() { return "ble"; } };
    struct op_bls { std::string operator()() { return "bls"; } };
    struct op_blt { std::string operator()() { return "blt"; } };
    struct op_bmi { std::string operator()() { return "bmi"; } };
    struct op_bne { std::string operator()() { return "bne"; } };
    struct op_bpl { std::string operator()() { return "bpl"; } };
    struct op_bra { std::string operator()() { return "bra"; } };
    struct op_brn { std::string operator()() { return "brn"; } };
    struct op_bvc { std::string operator()() { return "bvc"; } };
    struct op_bvs { std::string operator()() { return "bvs"; } };
    struct op_clr { std::string operator()() { return "clr"; } };
    struct op_clra { std::string operator()() { return "clra"; } };
    struct op_clrb { std::string operator()() { return "clrb"; } };
    struct op_cmpa { std::string operator()() { return "cmpa"; } };
    struct op_cmpb { std::string operator()() { return "cmpb"; } };
    struct op_cmpd { std::string operator()() { return "cmpd"; } };
    struct op_cmps { std::string operator()() { return "cmps"; } };
    struct op_cmpu { std::string operator()() { return "cmpu"; } };
    struct op_cmpx { std::string operator()() { return "cmpx"; } };
    struct op_cmpy { std::string operator()() { return "cmpy"; } };
    struct op_com { std::string operator()() { return "com"; } };
    struct op_coma { std::string operator()() { return "coma"; } };
    struct op_comb { std::string operator()() { return "comb"; } };
    struct op_cwai { std::string operator()() { return "cwai"; } };
    struct op_daa { std::string operator()() { return "daa"; } };
    struct op_dec { std::string operator()() { return "dec"; } };
    struct op_deca { std::string operator()() { return "deca"; } };
    struct op_decb { std::string operator()() { return "decb"; } };
    struct op_eora { std::string operator()() { return "eora"; } };
    struct op_eorb { std::string operator()() { return "eorb"; } };
    struct op_exg { std::string operator()() { return "exg"; } };
    struct op_inc { std::string operator()() { return "inc"; } };
    struct op_inca { std::string operator()() { return "inca"; } };
    struct op_incb { std::string operator()() { return "incb"; } };
    struct op_jmp { std::string operator()() { return "jmp"; } };
    struct op_jsr { std::string operator()() { return "jsr"; } };
    struct op_lbcc { std::string operator()() { return "lbcc"; } };
    struct op_lbcs { std::string operator()() { return "lbcs"; } };
    struct op_lbeq { std::string operator()() { return "lbeq"; } };
    struct op_lbge { std::string operator()() { return "lbge"; } };
    struct op_lbgt { std::string operator()() { return "lbgt"; } };
    struct op_lbhi { std::string operator()() { return "lbhi"; } };
    struct op_lble { std::string operator()() { return "lble"; } };
    struct op_lbls { std::string operator()() { return "lbls"; } };
    struct op_lblt { std::string operator()() { return "lblt"; } };
    struct op_lbmi { std::string operator()() { return "lbmi"; } };
    struct op_lbne { std::string operator()() { return "lbne"; } };
    struct op_lbpl { std::string operator()() { return "lbpl"; } };
    struct op_lbra { std::string operator()() { return "lbra"; } };
    struct op_lbrn { std::string operator()() { return "lbrn"; } };
    struct op_lbvc { std::string operator()() { return "lbvc"; } };
    struct op_lbvs { std::string operator()() { return "lbvs"; } };
    struct op_lda { std::string operator()() { return "lda"; } };
    struct op_ldb { std::string operator()() { return "ldb"; } };
    struct op_ldd { std::string operator()() { return "ldd"; } };
    struct op_lds { std::string operator()() { return "lds"; } };
    struct op_ldu { std::string operator()() { return "ldu"; } };
    struct op_ldx { std::string operator()() { return "ldx"; } };
    struct op_ldy { std::string operator()() { return "ldy"; } };
    struct op_leas { std::string operator()() { return "leas"; } };
    struct op_leau { std::string operator()() { return "leau"; } };
    struct op_leax { std::string operator()() { return "leax"; } };
    struct op_leay { std::string operator()() { return "leay"; } };
    struct op_lsl { std::string operator()() { return "lsl"; } };
    struct op_lsla { std::string operator()() { return "lsla"; } };
    struct op_lslb { std::string operator()() { return "lslb"; } };
    struct op_lsr { std::string operator()() { return "lsr"; } };
    struct op_lsra { std::string operator()() { return "lsra"; } };
    struct op_lsrb { std::string operator()() { return "lsrb"; } };
    struct op_mul { std::string operator()() { return "mul"; } };
    struct op_neg { std::string operator()() { return "neg"; } };
    struct op_nega { std::string operator()() { return "nega"; } };
    struct op_negb { std::string operator()() { return "negb"; } };
    struct op_nop { std::string operator()() { return "nop"; } };
    struct op_ora { std::string operator()() { return "ora"; } };
    struct op_orb { std::string operator()() { return "orb"; } };
    struct op_orcc { std::string operator()() { return "orcc"; } };
    struct op_pshs { std::string operator()() { return "pshs"; } };
    struct op_pshu { std::string operator()() { return "pshu"; } };
    struct op_puls { std::string operator()() { return "puls"; } };
    struct op_pulu { std::string operator()() { return "pulu"; } };
    struct op_rol { std::string operator()() { return "rol"; } };
    struct op_rola { std::string operator()() { return "rola"; } };
    struct op_rolb { std::string operator()() { return "rolb"; } };
    struct op_ror { std::string operator()() { return "ror"; } };
    struct op_rora { std::string operator()() { return "rora"; } };
    struct op_rorb { std::string operator()() { return "rorb"; } };
    struct op_rti { std::string operator()() { return "rti"; } };
    struct op_rts { std::string operator()() { return "rts"; } };
    struct op_sbca { std::string operator()() { return "sbca"; } };
    struct op_sbcb { std::string operator()() { return "sbcb"; } };
    struct op_sex { std::string operator()() { return "sex"; } };
    struct op_sta { std::string operator()() { return "sta"; } };
    struct op_stb { std::string operator()() { return "stb"; } };
    struct op_std { std::string operator()() { return "std"; } };
    struct op_sts { std::string operator()() { return "sts"; } };
    struct op_stu { std::string operator()() { return "stu"; } };
    struct op_stx { std::string operator()() { return "stx"; } };
    struct op_sty { std::string operator()() { return "sty"; } };
    struct op_suba { std::string operator()() { return "suba"; } };
    struct op_subb { std::string operator()() { return "subb"; } };
    struct op_subd { std::string operator()() { return "subd"; } };
    struct op_swi1 { std::string operator()() { return "swi1"; } };
    struct op_swi2 { std::string operator()() { return "swi2"; } };
    struct op_swi3 { std::string operator()() { return "swi3"; } };
    struct op_sync { std::string operator()() { return "sync"; } };
    struct op_tfr { std::string operator()() { return "tfr"; } };
    struct op_tst { std::string operator()() { return "tst"; } };
    struct op_tsta { std::string operator()() { return "tsta"; } };
    struct op_tstb { std::string operator()() { return "tstb"; } };
    struct op_bsr { std::string operator()() { return "bsr"; } };
    struct op_lbsr { std::string operator()() { return "lbsr"; } };

    struct DirectAddressing {
        M6809Operand operator()(M6809Disassemble& dis, uint16_t &addr)
        {
			M6809Operand operand(M6809OperandType::DIRECT);
			operand.directAddress = dis.Read8<uint8_t>(addr++);
			return operand;
        }
    };

    struct InherentAddressing { M6809Operand operator()(M6809Disassemble& dis, uint16_t &addr) { return M6809Operand(M6809OperandType::INHERENT); } };
    template <typename T>
    struct RelativeAddressing {
        M6809Operand operator()(M6809Disassemble& dis, uint16_t& addr)
        {
            M6809Operand operand(M6809OperandType::REL);
            operand.relativeAddress.offset = dis.Read<T>(addr);
            addr += sizeof(T);
            operand.relativeAddress.base = addr;
            return operand;
        }
    };
    using RelativeAddressingShort = RelativeAddressing<int8_t>;
    using RelativeAddressingLong  = RelativeAddressing<int16_t>;

    template<typename T>
    struct ImmediateAddressing {
        M6809Operand operator()(M6809Disassemble& dis, uint16_t &addr)
        {
            M6809Operand operand(M6809OperandType::IMMEDIATE);
            operand.immediate = dis.Read<T>(addr);
            addr += sizeof(T);
            return operand;
        }
    };
    using ImmediateAddressing8  = ImmediateAddressing<uint8_t>;
    using ImmediateAddressing16 = ImmediateAddressing<uint16_t>;

    struct ExtendedAddressing {
        M6809Operand operator()(M6809Disassemble& dis, uint16_t &addr)
        {
			M6809Operand operand(M6809OperandType::EXTENDED);
			operand.extendedAddress = dis.Read<uint16_t>(addr);
            addr += 2;
            return operand;
        }
    };

    struct IndexedAddressing {
        M6809Operand operator()(M6809Disassemble& dis, uint16_t &addr)
        {
            M6809Operand operand(M6809OperandType::INDEXED);
            uint8_t post_byte = dis.Read<uint8_t>(addr++);
            const auto reg = dis.index_mode_register_table[(post_byte >> 5) & 0x03];  // bits 5+6
            operand.indexed.indexReg = reg;

            if (!(post_byte >> 7))
            {
                // (+/- 4 bit offset),R
				operand.indexed.offset = (int8_t)((post_byte & 0xf) - (post_byte & 0x10));
                return operand;
            }
            else
            {
                switch (post_byte & 0x0f)
                {
                    case 0:
						// ,R+ post-increment
						operand.indexed.increment = 1;
                        break;
                    case 1:
                        // ,R++
						operand.indexed.increment = 2;
                        break;
                    case 2:
                        // ,-R
						operand.indexed.increment = -1;
                        break;
                    case 3:
                        // ,--R
						operand.indexed.increment = -2;
                        break;
                    case 4:
                        // ,R
						operand.indexed.offset = 0;
                        break;
                    case 5:
                        // (+/- B), R
						operand.indexed.offsetReg = M6809Register::B;
                        break;
                    case 6:
                        // (+/- A), R
                        operand.indexed.offsetReg = M6809Register::A;
                        break;
                    case 8:
                        // (+/- 7 bit offset), R
						operand.indexed.offset = dis.Read<int8_t>(addr++);
                        break;
                    case 9:
                        // (+/- 15 bit offset), R
						operand.indexed.offset = dis.Read<int16_t>(addr);
                        addr += 2;
                        break;
                    case 0xb:
                        // (+/- D), R
                        operand.indexed.offsetReg = M6809Register::D;
                        operand.indexed.indexReg = reg;
                        break;
                    case 0xc:
                        // (+/- 7 bit offset), PC
						operand.indexed.offset = dis.Read<int8_t>(addr++);
						operand.indexed.indexReg = M6809Register::PC;
                        break;
                    case 0xd:
                        // (+/- 15 bit offset), PC
						operand.indexed.offset = dis.Read<int16_t>(addr);
						operand.indexed.indexReg = M6809Register::PC;
                        addr += 2;
                        break;
                    case 0xf:
						operand.indexed.indirect = true;
                        operand.indexed.offset = dis.Read<int16_t>(addr);
                        operand.indexed.indexReg = M6809Register::INVALID;
                        addr += 2;
                        break;
                    default:
                        // Illegal
						throw std::exception("Illegal indexed addressing mode");
                }
                // indirect mode
                operand.indexed.indirect = ((post_byte >> 4) & 1);
                return operand;
            }

        }
    };

    template<typename mnemonic, typename Addressing>
    struct opcode
    {
        M6809Instruction operator()(M6809Disassemble& dis, uint16_t &addr)
        {
            return M6809Instruction(0, std::string(mnemonic()()), std::vector<M6809Operand>(1, Addressing()(dis, addr)));
        }
    };

    template <typename mnemonic>
    static M6809Instruction disasm_exg_tbl(M6809Disassemble& dis, uint16_t &addr)
    {
        M6809Operand reg_a(M6809OperandType::REG);
        M6809Operand reg_b(M6809OperandType::REG);
        auto post_byte = dis.Read<uint8_t>(addr++);
        reg_a.reg = dis.exg_register_table[(post_byte >> 4) & 0xf];
        reg_b.reg = dis.exg_register_table[post_byte & 0x0f];

        return M6809Instruction(0, mnemonic()(), std::vector<M6809Operand>({reg_a, reg_b}));
    }

    template<typename Op>
    static M6809Instruction opcodewrap(M6809Disassemble& dis, uint16_t &addr)
    {
        return Op()(dis, addr);
    }

    static M6809Instruction disasm_page1(M6809Disassemble& dis, uint16_t &addr);
    static M6809Instruction disasm_page2(M6809Disassemble& dis, uint16_t &addr);

public:
    M6809Disassemble();
    M6809Instruction disasm(uint16_t addr);
    void SetReadCallback(read_callback_t func, intptr_t ref);

    template<typename T>
    inline T Read8(const uint16_t& addr)
    {
        if (read_callback_func)
            return read_callback_func(read_callback_ref, addr);
        return 0;
    }

    template<typename T>
    inline T Read16(const uint16_t& addr)
    {
        return Read8<uint16_t>(addr) << 8 | Read8<uint16_t>(addr + 1);
    }
    

    template<typename T>
    inline T Read(const uint16_t& addr)
    {
        if (sizeof(T) == 1) {
            return Read8<T>(addr);
        }
        else
        {
            return Read16<T>(addr);
        }
    }

};


#endif //VECTREXIA_M6809_DISASSEMBLE_H
