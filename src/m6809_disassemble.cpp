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
#include <cstdarg>
#include <string>
#include <memory>
#include <bitset>
#include "m6809_disassemble.h"


std::string M6809Disassemble::disasm(uint16_t &addr)
{
    std::string addr_ = str_format("$%04x: ", addr);
    std::string instr;
    auto opcode = Read8(addr++);

    disasm_handler_t disasm_handler = this->disasm_handlers[opcode];
    if (disasm_handler)
    {
        instr = disasm_handler(*this, addr);
    }
    else
    {
        instr = str_format("ILLEGAL: %02x", opcode);
    }

    return addr_ + instr;
}

M6809Disassemble::M6809Disassemble()
{
    disasm_handlers.fill(nullptr);
    disasm_handlers_page1.fill(nullptr);
    disasm_handlers_page2.fill(nullptr);
    disasm_handlers[0x10] = std::addressof(disasm_page1);
    disasm_handlers[0x11] = std::addressof(disasm_page2);

    disasm_handlers[0x3A] = std::addressof(opcodewrap<opcode<op_abx, InherentAddressing>>);
    disasm_handlers[0x99] = std::addressof(opcodewrap<opcode<op_adca, DirectAddressing>>);
    disasm_handlers[0xB9] = std::addressof(opcodewrap<opcode<op_adca, ExtendedAddressing>>);
    disasm_handlers[0x89] = std::addressof(opcodewrap<opcode<op_adca, ImmediateAddressing8>>);
    disasm_handlers[0xA9] = std::addressof(opcodewrap<opcode<op_adca, IndexedAddressing>>);
    disasm_handlers[0xD9] = std::addressof(opcodewrap<opcode<op_adcb, DirectAddressing>>);
    disasm_handlers[0xF9] = std::addressof(opcodewrap<opcode<op_adcb, ExtendedAddressing>>);
    disasm_handlers[0xC9] = std::addressof(opcodewrap<opcode<op_adcb, ImmediateAddressing8>>);
    disasm_handlers[0xE9] = std::addressof(opcodewrap<opcode<op_adcb, IndexedAddressing>>);
    disasm_handlers[0x9B] = std::addressof(opcodewrap<opcode<op_adda, DirectAddressing>>);
    disasm_handlers[0xBB] = std::addressof(opcodewrap<opcode<op_adda, ExtendedAddressing>>);
    disasm_handlers[0x8B] = std::addressof(opcodewrap<opcode<op_adda, ImmediateAddressing8>>);
    disasm_handlers[0xAB] = std::addressof(opcodewrap<opcode<op_adda, IndexedAddressing>>);
    disasm_handlers[0xDB] = std::addressof(opcodewrap<opcode<op_addb, DirectAddressing>>);
    disasm_handlers[0xFB] = std::addressof(opcodewrap<opcode<op_addb, ExtendedAddressing>>);
    disasm_handlers[0xCB] = std::addressof(opcodewrap<opcode<op_addb, ImmediateAddressing8>>);
    disasm_handlers[0xEB] = std::addressof(opcodewrap<opcode<op_addb, IndexedAddressing>>);
    disasm_handlers[0xD3] = std::addressof(opcodewrap<opcode<op_addd, DirectAddressing>>);
    disasm_handlers[0xF3] = std::addressof(opcodewrap<opcode<op_addd, ExtendedAddressing>>);
    disasm_handlers[0xC3] = std::addressof(opcodewrap<opcode<op_addd, ImmediateAddressing16>>);
    disasm_handlers[0xE3] = std::addressof(opcodewrap<opcode<op_addd, IndexedAddressing>>);
    disasm_handlers[0x94] = std::addressof(opcodewrap<opcode<op_anda, DirectAddressing>>);
    disasm_handlers[0xB4] = std::addressof(opcodewrap<opcode<op_anda, ExtendedAddressing>>);
    disasm_handlers[0x84] = std::addressof(opcodewrap<opcode<op_anda, ImmediateAddressing8>>);
    disasm_handlers[0xA4] = std::addressof(opcodewrap<opcode<op_anda, IndexedAddressing>>);
    disasm_handlers[0xD4] = std::addressof(opcodewrap<opcode<op_andb, DirectAddressing>>);
    disasm_handlers[0xF4] = std::addressof(opcodewrap<opcode<op_andb, ExtendedAddressing>>);
    disasm_handlers[0xC4] = std::addressof(opcodewrap<opcode<op_andb, ImmediateAddressing8>>);
    disasm_handlers[0xE4] = std::addressof(opcodewrap<opcode<op_andb, IndexedAddressing>>);
    disasm_handlers[0x1C] = std::addressof(opcodewrap<opcode<op_andcc, ImmediateAddressing8>>);
    disasm_handlers[0x07] = std::addressof(opcodewrap<opcode<op_asr, DirectAddressing>>);
    disasm_handlers[0x77] = std::addressof(opcodewrap<opcode<op_asr, ExtendedAddressing>>);
    disasm_handlers[0x67] = std::addressof(opcodewrap<opcode<op_asr, IndexedAddressing>>);
    disasm_handlers[0x47] = std::addressof(opcodewrap<opcode<op_asra, InherentAddressing>>);
    disasm_handlers[0x57] = std::addressof(opcodewrap<opcode<op_asrb, InherentAddressing>>);
    disasm_handlers[0x95] = std::addressof(opcodewrap<opcode<op_bita, DirectAddressing>>);
    disasm_handlers[0xB5] = std::addressof(opcodewrap<opcode<op_bita, ExtendedAddressing>>);
    disasm_handlers[0x85] = std::addressof(opcodewrap<opcode<op_bita, ImmediateAddressing8>>);
    disasm_handlers[0xA5] = std::addressof(opcodewrap<opcode<op_bita, IndexedAddressing>>);
    disasm_handlers[0xD5] = std::addressof(opcodewrap<opcode<op_bitb, DirectAddressing>>);
    disasm_handlers[0xF5] = std::addressof(opcodewrap<opcode<op_bitb, ExtendedAddressing>>);
    disasm_handlers[0xC5] = std::addressof(opcodewrap<opcode<op_bitb, ImmediateAddressing8>>);
    disasm_handlers[0xE5] = std::addressof(opcodewrap<opcode<op_bitb, IndexedAddressing>>);
    disasm_handlers[0x0F] = std::addressof(opcodewrap<opcode<op_clr, DirectAddressing>>);
    disasm_handlers[0x7F] = std::addressof(opcodewrap<opcode<op_clr, ExtendedAddressing>>);
    disasm_handlers[0x6F] = std::addressof(opcodewrap<opcode<op_clr, IndexedAddressing>>);
    disasm_handlers[0x4F] = std::addressof(opcodewrap<opcode<op_clra, InherentAddressing>>);
    disasm_handlers[0x5F] = std::addressof(opcodewrap<opcode<op_clrb, InherentAddressing>>);
    disasm_handlers[0x91] = std::addressof(opcodewrap<opcode<op_cmpa, DirectAddressing>>);
    disasm_handlers[0xB1] = std::addressof(opcodewrap<opcode<op_cmpa, ExtendedAddressing>>);
    disasm_handlers[0x81] = std::addressof(opcodewrap<opcode<op_cmpa, ImmediateAddressing8>>);
    disasm_handlers[0xA1] = std::addressof(opcodewrap<opcode<op_cmpa, IndexedAddressing>>);
    disasm_handlers[0xD1] = std::addressof(opcodewrap<opcode<op_cmpb, DirectAddressing>>);
    disasm_handlers[0xF1] = std::addressof(opcodewrap<opcode<op_cmpb, ExtendedAddressing>>);
    disasm_handlers[0xC1] = std::addressof(opcodewrap<opcode<op_cmpb, ImmediateAddressing8>>);
    disasm_handlers[0xE1] = std::addressof(opcodewrap<opcode<op_cmpb, IndexedAddressing>>);
    disasm_handlers[0x9C] = std::addressof(opcodewrap<opcode<op_cmpx, DirectAddressing>>);
    disasm_handlers[0xBC] = std::addressof(opcodewrap<opcode<op_cmpx, ExtendedAddressing>>);
    disasm_handlers[0x8C] = std::addressof(opcodewrap<opcode<op_cmpx, ImmediateAddressing16>>);
    disasm_handlers[0xAC] = std::addressof(opcodewrap<opcode<op_cmpx, IndexedAddressing>>);
    disasm_handlers[0x03] = std::addressof(opcodewrap<opcode<op_com, DirectAddressing>>);
    disasm_handlers[0x73] = std::addressof(opcodewrap<opcode<op_com, ExtendedAddressing>>);
    disasm_handlers[0x63] = std::addressof(opcodewrap<opcode<op_com, IndexedAddressing>>);
    disasm_handlers[0x43] = std::addressof(opcodewrap<opcode<op_coma, InherentAddressing>>);
    disasm_handlers[0x53] = std::addressof(opcodewrap<opcode<op_comb, InherentAddressing>>);
    disasm_handlers[0x3C] = std::addressof(opcodewrap<opcode<op_cwai, ImmediateAddressing8>>);
    disasm_handlers[0x19] = std::addressof(opcodewrap<opcode<op_daa, InherentAddressing>>);
    disasm_handlers[0x0A] = std::addressof(opcodewrap<opcode<op_dec, DirectAddressing>>);
    disasm_handlers[0x7A] = std::addressof(opcodewrap<opcode<op_dec, ExtendedAddressing>>);
    disasm_handlers[0x6A] = std::addressof(opcodewrap<opcode<op_dec, IndexedAddressing>>);
    disasm_handlers[0x4A] = std::addressof(opcodewrap<opcode<op_deca, InherentAddressing>>);
    disasm_handlers[0x5A] = std::addressof(opcodewrap<opcode<op_decb, InherentAddressing>>);
    disasm_handlers[0x98] = std::addressof(opcodewrap<opcode<op_eora, DirectAddressing>>);
    disasm_handlers[0xB8] = std::addressof(opcodewrap<opcode<op_eora, ExtendedAddressing>>);
    disasm_handlers[0x88] = std::addressof(opcodewrap<opcode<op_eora, ImmediateAddressing8>>);
    disasm_handlers[0xA8] = std::addressof(opcodewrap<opcode<op_eora, IndexedAddressing>>);
    disasm_handlers[0xD8] = std::addressof(opcodewrap<opcode<op_eorb, DirectAddressing>>);
    disasm_handlers[0xF8] = std::addressof(opcodewrap<opcode<op_eorb, ExtendedAddressing>>);
    disasm_handlers[0xC8] = std::addressof(opcodewrap<opcode<op_eorb, ImmediateAddressing8>>);
    disasm_handlers[0xE8] = std::addressof(opcodewrap<opcode<op_eorb, IndexedAddressing>>);
    disasm_handlers[0x1E] = std::addressof(disasm_exg_tbl<op_exg>);
    disasm_handlers[0x0C] = std::addressof(opcodewrap<opcode<op_inc, DirectAddressing>>);
    disasm_handlers[0x7C] = std::addressof(opcodewrap<opcode<op_inc, ExtendedAddressing>>);
    disasm_handlers[0x6C] = std::addressof(opcodewrap<opcode<op_inc, IndexedAddressing>>);
    disasm_handlers[0x4C] = std::addressof(opcodewrap<opcode<op_inca, InherentAddressing>>);
    disasm_handlers[0x5C] = std::addressof(opcodewrap<opcode<op_incb, InherentAddressing>>);
    disasm_handlers[0x0E] = std::addressof(opcodewrap<opcode<op_jmp, DirectAddressing>>);
    disasm_handlers[0x7E] = std::addressof(opcodewrap<opcode<op_jmp, ExtendedAddressing>>);
    disasm_handlers[0x6E] = std::addressof(opcodewrap<opcode<op_jmp, IndexedAddressing>>);
    disasm_handlers[0x9D] = std::addressof(opcodewrap<opcode<op_jsr, DirectAddressing>>);
    disasm_handlers[0xBD] = std::addressof(opcodewrap<opcode<op_jsr, ExtendedAddressing>>);
    disasm_handlers[0xAD] = std::addressof(opcodewrap<opcode<op_jsr, IndexedAddressing>>);
    disasm_handlers[0x96] = std::addressof(opcodewrap<opcode<op_lda, DirectAddressing>>);
    disasm_handlers[0xB6] = std::addressof(opcodewrap<opcode<op_lda, ExtendedAddressing>>);
    disasm_handlers[0x86] = std::addressof(opcodewrap<opcode<op_lda, ImmediateAddressing8>>);
    disasm_handlers[0xA6] = std::addressof(opcodewrap<opcode<op_lda, IndexedAddressing>>);
    disasm_handlers[0xD6] = std::addressof(opcodewrap<opcode<op_ldb, DirectAddressing>>);
    disasm_handlers[0xF6] = std::addressof(opcodewrap<opcode<op_ldb, ExtendedAddressing>>);
    disasm_handlers[0xC6] = std::addressof(opcodewrap<opcode<op_ldb, ImmediateAddressing8>>);
    disasm_handlers[0xE6] = std::addressof(opcodewrap<opcode<op_ldb, IndexedAddressing>>);
    disasm_handlers[0xDC] = std::addressof(opcodewrap<opcode<op_ldd, DirectAddressing>>);
    disasm_handlers[0xFC] = std::addressof(opcodewrap<opcode<op_ldd, ExtendedAddressing>>);
    disasm_handlers[0xCC] = std::addressof(opcodewrap<opcode<op_ldd, ImmediateAddressing16>>);
    disasm_handlers[0xEC] = std::addressof(opcodewrap<opcode<op_ldd, IndexedAddressing>>);
    disasm_handlers[0xDE] = std::addressof(opcodewrap<opcode<op_ldu, DirectAddressing>>);
    disasm_handlers[0xFE] = std::addressof(opcodewrap<opcode<op_ldu, ExtendedAddressing>>);
    disasm_handlers[0xCE] = std::addressof(opcodewrap<opcode<op_ldu, ImmediateAddressing16>>);
    disasm_handlers[0xEE] = std::addressof(opcodewrap<opcode<op_ldu, IndexedAddressing>>);
    disasm_handlers[0x9E] = std::addressof(opcodewrap<opcode<op_ldx, DirectAddressing>>);
    disasm_handlers[0xBE] = std::addressof(opcodewrap<opcode<op_ldx, ExtendedAddressing>>);
    disasm_handlers[0x8E] = std::addressof(opcodewrap<opcode<op_ldx, ImmediateAddressing16>>);
    disasm_handlers[0xAE] = std::addressof(opcodewrap<opcode<op_ldx, IndexedAddressing>>);
    disasm_handlers[0x32] = std::addressof(opcodewrap<opcode<op_leas, IndexedAddressing>>);
    disasm_handlers[0x33] = std::addressof(opcodewrap<opcode<op_leau, IndexedAddressing>>);
    disasm_handlers[0x30] = std::addressof(opcodewrap<opcode<op_leax, IndexedAddressing>>);
    disasm_handlers[0x31] = std::addressof(opcodewrap<opcode<op_leay, IndexedAddressing>>);
    disasm_handlers[0x08] = std::addressof(opcodewrap<opcode<op_lsl, DirectAddressing>>);
    disasm_handlers[0x78] = std::addressof(opcodewrap<opcode<op_lsl, ExtendedAddressing>>);
    disasm_handlers[0x68] = std::addressof(opcodewrap<opcode<op_lsl, IndexedAddressing>>);
    disasm_handlers[0x48] = std::addressof(opcodewrap<opcode<op_lsla, InherentAddressing>>);
    disasm_handlers[0x58] = std::addressof(opcodewrap<opcode<op_lslb, InherentAddressing>>);
    disasm_handlers[0x04] = std::addressof(opcodewrap<opcode<op_lsr, DirectAddressing>>);
    disasm_handlers[0x74] = std::addressof(opcodewrap<opcode<op_lsr, ExtendedAddressing>>);
    disasm_handlers[0x64] = std::addressof(opcodewrap<opcode<op_lsr, IndexedAddressing>>);
    disasm_handlers[0x44] = std::addressof(opcodewrap<opcode<op_lsra, InherentAddressing>>);
    disasm_handlers[0x54] = std::addressof(opcodewrap<opcode<op_lsrb, InherentAddressing>>);
    disasm_handlers[0x3D] = std::addressof(opcodewrap<opcode<op_mul, InherentAddressing>>);
    disasm_handlers[0x00] = std::addressof(opcodewrap<opcode<op_neg, DirectAddressing>>);
    disasm_handlers[0x70] = std::addressof(opcodewrap<opcode<op_neg, ExtendedAddressing>>);
    disasm_handlers[0x60] = std::addressof(opcodewrap<opcode<op_neg, IndexedAddressing>>);
    disasm_handlers[0x40] = std::addressof(opcodewrap<opcode<op_nega, InherentAddressing>>);
    disasm_handlers[0x50] = std::addressof(opcodewrap<opcode<op_negb, InherentAddressing>>);
    disasm_handlers[0x12] = std::addressof(opcodewrap<opcode<op_nop, InherentAddressing>>);
    disasm_handlers[0x9A] = std::addressof(opcodewrap<opcode<op_ora, DirectAddressing>>);
    disasm_handlers[0xBA] = std::addressof(opcodewrap<opcode<op_ora, ExtendedAddressing>>);
    disasm_handlers[0x8A] = std::addressof(opcodewrap<opcode<op_ora, ImmediateAddressing8>>);
    disasm_handlers[0xAA] = std::addressof(opcodewrap<opcode<op_ora, IndexedAddressing>>);
    disasm_handlers[0xDA] = std::addressof(opcodewrap<opcode<op_orb, DirectAddressing>>);
    disasm_handlers[0xFA] = std::addressof(opcodewrap<opcode<op_orb, ExtendedAddressing>>);
    disasm_handlers[0xCA] = std::addressof(opcodewrap<opcode<op_orb, ImmediateAddressing8>>);
    disasm_handlers[0xEA] = std::addressof(opcodewrap<opcode<op_orb, IndexedAddressing>>);
    disasm_handlers[0x1A] = std::addressof(opcodewrap<opcode<op_orcc, ImmediateAddressing8>>);
    disasm_handlers[0x34] = std::addressof(opcodewrap<opcode<op_pshs, ImmediateAddressing8>>);
    disasm_handlers[0x36] = std::addressof(opcodewrap<opcode<op_pshu, ImmediateAddressing8>>);
    disasm_handlers[0x35] = std::addressof(opcodewrap<opcode<op_puls, ImmediateAddressing8>>);
    disasm_handlers[0x37] = std::addressof(opcodewrap<opcode<op_pulu, ImmediateAddressing8>>);
    disasm_handlers[0x09] = std::addressof(opcodewrap<opcode<op_rol, DirectAddressing>>);
    disasm_handlers[0x79] = std::addressof(opcodewrap<opcode<op_rol, ExtendedAddressing>>);
    disasm_handlers[0x69] = std::addressof(opcodewrap<opcode<op_rol, IndexedAddressing>>);
    disasm_handlers[0x49] = std::addressof(opcodewrap<opcode<op_rola, InherentAddressing>>);
    disasm_handlers[0x59] = std::addressof(opcodewrap<opcode<op_rolb, InherentAddressing>>);
    disasm_handlers[0x06] = std::addressof(opcodewrap<opcode<op_ror, DirectAddressing>>);
    disasm_handlers[0x76] = std::addressof(opcodewrap<opcode<op_ror, ExtendedAddressing>>);
    disasm_handlers[0x66] = std::addressof(opcodewrap<opcode<op_ror, IndexedAddressing>>);
    disasm_handlers[0x46] = std::addressof(opcodewrap<opcode<op_rora, InherentAddressing>>);
    disasm_handlers[0x56] = std::addressof(opcodewrap<opcode<op_rorb, InherentAddressing>>);
    disasm_handlers[0x3B] = std::addressof(opcodewrap<opcode<op_rti, InherentAddressing>>);
    disasm_handlers[0x39] = std::addressof(opcodewrap<opcode<op_rts, InherentAddressing>>);
    disasm_handlers[0x92] = std::addressof(opcodewrap<opcode<op_sbca, DirectAddressing>>);
    disasm_handlers[0xB2] = std::addressof(opcodewrap<opcode<op_sbca, ExtendedAddressing>>);
    disasm_handlers[0x82] = std::addressof(opcodewrap<opcode<op_sbca, ImmediateAddressing8>>);
    disasm_handlers[0xA2] = std::addressof(opcodewrap<opcode<op_sbca, IndexedAddressing>>);
    disasm_handlers[0xD2] = std::addressof(opcodewrap<opcode<op_sbcb, DirectAddressing>>);
    disasm_handlers[0xF2] = std::addressof(opcodewrap<opcode<op_sbcb, ExtendedAddressing>>);
    disasm_handlers[0xC2] = std::addressof(opcodewrap<opcode<op_sbcb, ImmediateAddressing8>>);
    disasm_handlers[0xE2] = std::addressof(opcodewrap<opcode<op_sbcb, IndexedAddressing>>);
    disasm_handlers[0x1D] = std::addressof(opcodewrap<opcode<op_sex, InherentAddressing>>);
    disasm_handlers[0x97] = std::addressof(opcodewrap<opcode<op_sta, DirectAddressing>>);
    disasm_handlers[0xB7] = std::addressof(opcodewrap<opcode<op_sta, ExtendedAddressing>>);
    disasm_handlers[0xA7] = std::addressof(opcodewrap<opcode<op_sta, IndexedAddressing>>);
    disasm_handlers[0xD7] = std::addressof(opcodewrap<opcode<op_stb, DirectAddressing>>);
    disasm_handlers[0xF7] = std::addressof(opcodewrap<opcode<op_stb, ExtendedAddressing>>);
    disasm_handlers[0xE7] = std::addressof(opcodewrap<opcode<op_stb, IndexedAddressing>>);
    disasm_handlers[0xDD] = std::addressof(opcodewrap<opcode<op_std, DirectAddressing>>);
    disasm_handlers[0xFD] = std::addressof(opcodewrap<opcode<op_std, ExtendedAddressing>>);
    disasm_handlers[0xED] = std::addressof(opcodewrap<opcode<op_std, IndexedAddressing>>);
    disasm_handlers[0xDF] = std::addressof(opcodewrap<opcode<op_stu, DirectAddressing>>);
    disasm_handlers[0xFF] = std::addressof(opcodewrap<opcode<op_stu, ExtendedAddressing>>);
    disasm_handlers[0xEF] = std::addressof(opcodewrap<opcode<op_stu, IndexedAddressing>>);
    disasm_handlers[0x9F] = std::addressof(opcodewrap<opcode<op_stx, DirectAddressing>>);
    disasm_handlers[0xBF] = std::addressof(opcodewrap<opcode<op_stx, ExtendedAddressing>>);
    disasm_handlers[0xAF] = std::addressof(opcodewrap<opcode<op_stx, IndexedAddressing>>);
    disasm_handlers[0x90] = std::addressof(opcodewrap<opcode<op_suba, DirectAddressing>>);
    disasm_handlers[0xB0] = std::addressof(opcodewrap<opcode<op_suba, ExtendedAddressing>>);
    disasm_handlers[0x80] = std::addressof(opcodewrap<opcode<op_suba, ImmediateAddressing8>>);
    disasm_handlers[0xA0] = std::addressof(opcodewrap<opcode<op_suba, IndexedAddressing>>);
    disasm_handlers[0xD0] = std::addressof(opcodewrap<opcode<op_subb, DirectAddressing>>);
    disasm_handlers[0xF0] = std::addressof(opcodewrap<opcode<op_subb, ExtendedAddressing>>);
    disasm_handlers[0xC0] = std::addressof(opcodewrap<opcode<op_subb, ImmediateAddressing8>>);
    disasm_handlers[0xE0] = std::addressof(opcodewrap<opcode<op_subb, IndexedAddressing>>);
    disasm_handlers[0x93] = std::addressof(opcodewrap<opcode<op_subd, DirectAddressing>>);
    disasm_handlers[0xB3] = std::addressof(opcodewrap<opcode<op_subd, ExtendedAddressing>>);
    disasm_handlers[0x83] = std::addressof(opcodewrap<opcode<op_subd, ImmediateAddressing16>>);
    disasm_handlers[0xA3] = std::addressof(opcodewrap<opcode<op_subd, IndexedAddressing>>);
    disasm_handlers[0x3F] = std::addressof(opcodewrap<opcode<op_swi1, InherentAddressing>>);
    disasm_handlers[0x13] = std::addressof(opcodewrap<opcode<op_sync, InherentAddressing>>);
    disasm_handlers[0x1F] = std::addressof(disasm_exg_tbl<op_tfr>);
    disasm_handlers[0x0D] = std::addressof(opcodewrap<opcode<op_tst, DirectAddressing>>);
    disasm_handlers[0x7D] = std::addressof(opcodewrap<opcode<op_tst, ExtendedAddressing>>);
    disasm_handlers[0x6D] = std::addressof(opcodewrap<opcode<op_tst, IndexedAddressing>>);
    disasm_handlers[0x4D] = std::addressof(opcodewrap<opcode<op_tsta, InherentAddressing>>);
    disasm_handlers[0x5D] = std::addressof(opcodewrap<opcode<op_tstb, InherentAddressing>>);
    disasm_handlers[0x8D] = std::addressof(opcodewrap<opcode<op_bsr, RelativeAddressingShort>>);
    disasm_handlers[0x17] = std::addressof(opcodewrap<opcode<op_lbsr, RelativeAddressingLong>>);

    disasm_handlers_page1[0x3f] = std::addressof(opcodewrap<opcode<op_swi2, InherentAddressing>>);
    disasm_handlers_page2[0x3f] = std::addressof(opcodewrap<opcode<op_swi3, InherentAddressing>>);
    disasm_handlers_page1[0x9f] = std::addressof(opcodewrap<opcode<op_sty, DirectAddressing>>);
    disasm_handlers_page1[0xaf] = std::addressof(opcodewrap<opcode<op_sty, IndexedAddressing>>);
    disasm_handlers_page1[0xbf] = std::addressof(opcodewrap<opcode<op_sty, ExtendedAddressing>>);
    disasm_handlers_page1[0xdf] = std::addressof(opcodewrap<opcode<op_sts, DirectAddressing>>);
    disasm_handlers_page1[0xef] = std::addressof(opcodewrap<opcode<op_sts, IndexedAddressing>>);
    disasm_handlers_page1[0xff] = std::addressof(opcodewrap<opcode<op_sts, ExtendedAddressing>>);
    disasm_handlers_page1[0x83] = std::addressof(opcodewrap<opcode<op_cmpd, ImmediateAddressing16>>);
    disasm_handlers_page1[0x8c] = std::addressof(opcodewrap<opcode<op_cmpy, ImmediateAddressing16>>);
    disasm_handlers_page1[0x8e] = std::addressof(opcodewrap<opcode<op_ldy, ImmediateAddressing16>>);
    disasm_handlers_page1[0x93] = std::addressof(opcodewrap<opcode<op_cmpd, DirectAddressing>>);
    disasm_handlers_page1[0x9c] = std::addressof(opcodewrap<opcode<op_cmpy, DirectAddressing>>);
    disasm_handlers_page1[0x9e] = std::addressof(opcodewrap<opcode<op_ldy, DirectAddressing>>);
    disasm_handlers_page1[0xa3] = std::addressof(opcodewrap<opcode<op_cmpd, IndexedAddressing>>);
    disasm_handlers_page1[0xac] = std::addressof(opcodewrap<opcode<op_cmpy, IndexedAddressing>>);
    disasm_handlers_page1[0xae] = std::addressof(opcodewrap<opcode<op_ldy, IndexedAddressing>>);
    disasm_handlers_page1[0xb3] = std::addressof(opcodewrap<opcode<op_cmpd, ExtendedAddressing>>);
    disasm_handlers_page1[0xbc] = std::addressof(opcodewrap<opcode<op_cmpy, ExtendedAddressing>>);
    disasm_handlers_page1[0xbe] = std::addressof(opcodewrap<opcode<op_ldy, ExtendedAddressing>>);
    disasm_handlers_page1[0xce] = std::addressof(opcodewrap<opcode<op_lds, ImmediateAddressing16>>);
    disasm_handlers_page1[0xde] = std::addressof(opcodewrap<opcode<op_lds, DirectAddressing>>);
    disasm_handlers_page1[0xee] = std::addressof(opcodewrap<opcode<op_lds, IndexedAddressing>>);
    disasm_handlers_page1[0xfe] = std::addressof(opcodewrap<opcode<op_lds, ExtendedAddressing>>);
    disasm_handlers_page2[0x83] = std::addressof(opcodewrap<opcode<op_cmpu, ImmediateAddressing16>>);
    disasm_handlers_page2[0x8c] = std::addressof(opcodewrap<opcode<op_cmps, ImmediateAddressing16>>);
    disasm_handlers_page2[0x93] = std::addressof(opcodewrap<opcode<op_cmpu, DirectAddressing>>);
    disasm_handlers_page2[0x9c] = std::addressof(opcodewrap<opcode<op_cmps, DirectAddressing>>);
    disasm_handlers_page2[0xa3] = std::addressof(opcodewrap<opcode<op_cmpu, IndexedAddressing>>);
    disasm_handlers_page2[0xac] = std::addressof(opcodewrap<opcode<op_cmps, IndexedAddressing>>);
    disasm_handlers_page2[0xb3] = std::addressof(opcodewrap<opcode<op_cmpu, ExtendedAddressing>>);
    disasm_handlers_page2[0xbc] = std::addressof(opcodewrap<opcode<op_cmps, ExtendedAddressing>>);

    // branches
    disasm_handlers[0x20] = std::addressof(opcodewrap<opcode<op_bra, RelativeAddressingShort>>);
    disasm_handlers[0x16] = std::addressof(opcodewrap<opcode<op_lbra, RelativeAddressingLong>>);
    disasm_handlers[0x21] = std::addressof(opcodewrap<opcode<op_brn, RelativeAddressingShort>>);
    disasm_handlers_page1[0x21] = std::addressof(opcodewrap<opcode<op_lbrn, RelativeAddressingLong>>);
    disasm_handlers[0x25] = std::addressof(opcodewrap<opcode<op_bcs, RelativeAddressingShort>>);
    disasm_handlers_page1[0x25] = std::addressof(opcodewrap<opcode<op_lbcs, RelativeAddressingLong>>);
    disasm_handlers[0x24] = std::addressof(opcodewrap<opcode<op_bcc, RelativeAddressingShort>>);
    disasm_handlers_page1[0x24] = std::addressof(opcodewrap<opcode<op_lbcc, RelativeAddressingLong>>);
    disasm_handlers[0x22] = std::addressof(opcodewrap<opcode<op_bhi, RelativeAddressingShort>>);
    disasm_handlers_page1[0x22] = std::addressof(opcodewrap<opcode<op_lbhi, RelativeAddressingLong>>);
    disasm_handlers[0x23] = std::addressof(opcodewrap<opcode<op_bls, RelativeAddressingShort>>);
    disasm_handlers_page1[0x23] = std::addressof(opcodewrap<opcode<op_lbls, RelativeAddressingLong>>);
    disasm_handlers[0x27] = std::addressof(opcodewrap<opcode<op_beq, RelativeAddressingShort>>);
    disasm_handlers_page1[0x27] = std::addressof(opcodewrap<opcode<op_lbeq, RelativeAddressingLong>>);
    disasm_handlers[0x26] = std::addressof(opcodewrap<opcode<op_bne, RelativeAddressingShort>>);
    disasm_handlers_page1[0x26] = std::addressof(opcodewrap<opcode<op_lbne, RelativeAddressingLong>>);
    disasm_handlers[0x2E] = std::addressof(opcodewrap<opcode<op_bgt, RelativeAddressingShort>>);
    disasm_handlers_page1[0x2E] = std::addressof(opcodewrap<opcode<op_lbgt, RelativeAddressingLong>>);
    disasm_handlers[0x2D] = std::addressof(opcodewrap<opcode<op_blt, RelativeAddressingShort>>);
    disasm_handlers_page1[0x2D] = std::addressof(opcodewrap<opcode<op_lblt, RelativeAddressingLong>>);
    disasm_handlers[0x2C] = std::addressof(opcodewrap<opcode<op_bge, RelativeAddressingShort>>);
    disasm_handlers_page1[0x2C] = std::addressof(opcodewrap<opcode<op_lbge, RelativeAddressingLong>>);
    disasm_handlers[0x2F] = std::addressof(opcodewrap<opcode<op_ble, RelativeAddressingShort>>);
    disasm_handlers_page1[0x2F] = std::addressof(opcodewrap<opcode<op_lble, RelativeAddressingLong>>);
    disasm_handlers[0x2A] = std::addressof(opcodewrap<opcode<op_bpl, RelativeAddressingShort>>);
    disasm_handlers_page1[0x2A] = std::addressof(opcodewrap<opcode<op_lbpl, RelativeAddressingLong>>);
    disasm_handlers[0x2B] = std::addressof(opcodewrap<opcode<op_bmi, RelativeAddressingShort>>);
    disasm_handlers_page1[0x2B] = std::addressof(opcodewrap<opcode<op_lbmi, RelativeAddressingLong>>);
    disasm_handlers[0x29] = std::addressof(opcodewrap<opcode<op_bvs, RelativeAddressingShort>>);
    disasm_handlers_page1[0x29] = std::addressof(opcodewrap<opcode<op_lbvs, RelativeAddressingLong>>);
    disasm_handlers[0x28] = std::addressof(opcodewrap<opcode<op_bvc, RelativeAddressingShort>>);
    disasm_handlers_page1[0x28] = std::addressof(opcodewrap<opcode<op_lbvc, RelativeAddressingLong>>);
}

std::string M6809Disassemble::disasm_page1(M6809Disassemble &dis, uint16_t &addr)
{
    auto opcode = dis.Read8(addr++);
    disasm_handler_t page1_disasm = dis.disasm_handlers_page1[opcode];
    if (page1_disasm)
    {
        return page1_disasm(dis, addr);
    }
    else
    {
        return str_format("ILLEGAL PAGE1: %02x", opcode);
    }
}

std::string M6809Disassemble::disasm_page2(M6809Disassemble &dis, uint16_t &addr)
{
    auto opcode = dis.Read8(addr++);
    disasm_handler_t page2_disasm = dis.disasm_handlers_page2[opcode];
    if (page2_disasm)
    {
        return page2_disasm(dis, addr);
    }
    else
    {
        return str_format("ILLEGAL PAGE2: %02x", opcode);
    }
}

void M6809Disassemble::SetReadCallback(M6809Disassemble::read_callback_t func, intptr_t ref)
{
    read_callback_func = func;
    read_callback_ref = ref;
}

std::string M6809Disassemble::str_format(const char *fmt, ...)
{
    va_list ap, ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    std::string out;

    int size = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);
    if (size > 0) {
        out = std::string(size+1, 0);
        vsnprintf(&out[0], out.size(), fmt, ap2);
        out.resize(static_cast<size_t>(size));
    }

    va_end(ap2);
    return out;
}
