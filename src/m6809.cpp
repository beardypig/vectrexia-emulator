/*
Copyright (C) 2016 Beardypig

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
#include <memory>
#include "m6809.h"

M6809::M6809()
{
    opcode_handlers.fill(nullptr);
    opcode_handlers[0x3A] = std::addressof(opcodewrap<op_abx_inherent>);
    opcode_handlers[0x99] = std::addressof(opcodewrap<op_adca_direct>);
    opcode_handlers[0xB9] = std::addressof(opcodewrap<op_adca_extended>);
    opcode_handlers[0x89] = std::addressof(opcodewrap<op_adca_immediate>);
    opcode_handlers[0xA9] = std::addressof(opcodewrap<op_adca_indexed>);
    opcode_handlers[0xD9] = std::addressof(opcodewrap<op_adcb_direct>);
    opcode_handlers[0xF9] = std::addressof(opcodewrap<op_adcb_extended>);
    opcode_handlers[0xC9] = std::addressof(opcodewrap<op_adcb_immediate>);
    opcode_handlers[0xE9] = std::addressof(opcodewrap<op_adcb_indexed>);
    opcode_handlers[0x9B] = std::addressof(opcodewrap<op_adda_direct>);
    opcode_handlers[0xBB] = std::addressof(opcodewrap<op_adda_extended>);
    opcode_handlers[0x8B] = std::addressof(opcodewrap<op_adda_immediate>);
    opcode_handlers[0xAB] = std::addressof(opcodewrap<op_adda_indexed>);
    opcode_handlers[0xDB] = std::addressof(opcodewrap<op_addb_direct>);
    opcode_handlers[0xFB] = std::addressof(opcodewrap<op_addb_extended>);
    opcode_handlers[0xCB] = std::addressof(opcodewrap<op_addb_immediate>);
    opcode_handlers[0xEB] = std::addressof(opcodewrap<op_addb_indexed>);
    opcode_handlers[0xD3] = std::addressof(opcodewrap<op_addd_direct>);
    opcode_handlers[0xF3] = std::addressof(opcodewrap<op_addd_extended>);
    opcode_handlers[0xC3] = std::addressof(opcodewrap<op_addd_immediate>);
    opcode_handlers[0xE3] = std::addressof(opcodewrap<op_addd_indexed>);
    opcode_handlers[0x94] = std::addressof(opcodewrap<op_anda_direct>);
    opcode_handlers[0xB4] = std::addressof(opcodewrap<op_anda_extended>);
    opcode_handlers[0x84] = std::addressof(opcodewrap<op_anda_immediate>);
    opcode_handlers[0xA4] = std::addressof(opcodewrap<op_anda_indexed>);
    opcode_handlers[0xD4] = std::addressof(opcodewrap<op_andb_direct>);
    opcode_handlers[0xF4] = std::addressof(opcodewrap<op_andb_extended>);
    opcode_handlers[0xC4] = std::addressof(opcodewrap<op_andb_immediate>);
    opcode_handlers[0xE4] = std::addressof(opcodewrap<op_andb_indexed>);
    opcode_handlers[0x1C] = std::addressof(opcodewrap<op_andcc_immediate>);
    opcode_handlers[0x07] = std::addressof(opcodewrap<op_asr_direct>);
    opcode_handlers[0x77] = std::addressof(opcodewrap<op_asr_extended>);
    opcode_handlers[0x67] = std::addressof(opcodewrap<op_asr_indexed>);
    opcode_handlers[0x47] = std::addressof(opcodewrap<op_asra_inherent>);
    opcode_handlers[0x57] = std::addressof(opcodewrap<op_asrb_inherent>);
    opcode_handlers[0x95] = std::addressof(opcodewrap<op_bita_direct>);
    opcode_handlers[0xB5] = std::addressof(opcodewrap<op_bita_extended>);
    opcode_handlers[0x85] = std::addressof(opcodewrap<op_bita_immediate>);
    opcode_handlers[0xA5] = std::addressof(opcodewrap<op_bita_indexed>);
    opcode_handlers[0xD5] = std::addressof(opcodewrap<op_bitb_direct>);
    opcode_handlers[0xF5] = std::addressof(opcodewrap<op_bitb_extended>);
    opcode_handlers[0xC5] = std::addressof(opcodewrap<op_bitb_immediate>);
    opcode_handlers[0xE5] = std::addressof(opcodewrap<op_bitb_indexed>);
    opcode_handlers[0x0F] = std::addressof(opcodewrap<op_clr_direct>);
    opcode_handlers[0x7F] = std::addressof(opcodewrap<op_clr_extended>);
    opcode_handlers[0x6F] = std::addressof(opcodewrap<op_clr_indexed>);
    opcode_handlers[0x4F] = std::addressof(opcodewrap<op_clra_inherent>);
    opcode_handlers[0x5F] = std::addressof(opcodewrap<op_clrb_inherent>);
    opcode_handlers[0x91] = std::addressof(opcodewrap<op_cmpa_direct>);
    opcode_handlers[0xB1] = std::addressof(opcodewrap<op_cmpa_extended>);
    opcode_handlers[0x81] = std::addressof(opcodewrap<op_cmpa_immediate>);
    opcode_handlers[0xA1] = std::addressof(opcodewrap<op_cmpa_indexed>);
    opcode_handlers[0xD1] = std::addressof(opcodewrap<op_cmpb_direct>);
    opcode_handlers[0xF1] = std::addressof(opcodewrap<op_cmpb_extended>);
    opcode_handlers[0xC1] = std::addressof(opcodewrap<op_cmpb_immediate>);
    opcode_handlers[0xE1] = std::addressof(opcodewrap<op_cmpb_indexed>);
    opcode_handlers[0x9C] = std::addressof(opcodewrap<op_cmpx_direct>);
    opcode_handlers[0xBC] = std::addressof(opcodewrap<op_cmpx_extended>);
    opcode_handlers[0x8C] = std::addressof(opcodewrap<op_cmpx_immediate>);
    opcode_handlers[0xAC] = std::addressof(opcodewrap<op_cmpx_indexed>);
    opcode_handlers[0x03] = std::addressof(opcodewrap<op_com_direct>);
    opcode_handlers[0x73] = std::addressof(opcodewrap<op_com_extended>);
    opcode_handlers[0x63] = std::addressof(opcodewrap<op_com_indexed>);
    opcode_handlers[0x43] = std::addressof(opcodewrap<op_coma_inherent>);
    opcode_handlers[0x53] = std::addressof(opcodewrap<op_comb_inherent>);
    opcode_handlers[0x3C] = std::addressof(opcodewrap<op_cwai_immediate>);
    opcode_handlers[0x19] = std::addressof(opcodewrap<op_daa_inherent>);
    opcode_handlers[0x0A] = std::addressof(opcodewrap<op_dec_direct>);
    opcode_handlers[0x7A] = std::addressof(opcodewrap<op_dec_extended>);
    opcode_handlers[0x6A] = std::addressof(opcodewrap<op_dec_indexed>);
    opcode_handlers[0x4A] = std::addressof(opcodewrap<op_deca_inherent>);
    opcode_handlers[0x5A] = std::addressof(opcodewrap<op_decb_inherent>);
    opcode_handlers[0x98] = std::addressof(opcodewrap<op_eora_direct>);
    opcode_handlers[0xB8] = std::addressof(opcodewrap<op_eora_extended>);
    opcode_handlers[0x88] = std::addressof(opcodewrap<op_eora_immediate>);
    opcode_handlers[0xA8] = std::addressof(opcodewrap<op_eora_indexed>);
    opcode_handlers[0xD8] = std::addressof(opcodewrap<op_eorb_direct>);
    opcode_handlers[0xF8] = std::addressof(opcodewrap<op_eorb_extended>);
    opcode_handlers[0xC8] = std::addressof(opcodewrap<op_eorb_immediate>);
    opcode_handlers[0xE8] = std::addressof(opcodewrap<op_eorb_indexed>);
    opcode_handlers[0x1E] = std::addressof(opcodewrap<op_exg_immediate>);
    opcode_handlers[0x0C] = std::addressof(opcodewrap<op_inc_direct>);
    opcode_handlers[0x7C] = std::addressof(opcodewrap<op_inc_extended>);
    opcode_handlers[0x6C] = std::addressof(opcodewrap<op_inc_indexed>);
    opcode_handlers[0x4C] = std::addressof(opcodewrap<op_inca_inherent>);
    opcode_handlers[0x5C] = std::addressof(opcodewrap<op_incb_inherent>);
    opcode_handlers[0x0E] = std::addressof(opcodewrap<op_jmp_direct>);
    opcode_handlers[0x7E] = std::addressof(opcodewrap<op_jmp_extended>);
    opcode_handlers[0x6E] = std::addressof(opcodewrap<op_jmp_indexed>);
    opcode_handlers[0x9D] = std::addressof(opcodewrap<op_jsr_direct>);
    opcode_handlers[0xBD] = std::addressof(opcodewrap<op_jsr_extended>);
    opcode_handlers[0xAD] = std::addressof(opcodewrap<op_jsr_indexed>);
    opcode_handlers[0x96] = std::addressof(opcodewrap<op_lda_direct>);
    opcode_handlers[0xB6] = std::addressof(opcodewrap<op_lda_extended>);
    opcode_handlers[0x86] = std::addressof(opcodewrap<op_lda_immediate>);
    opcode_handlers[0xA6] = std::addressof(opcodewrap<op_lda_indexed>);
    opcode_handlers[0xD6] = std::addressof(opcodewrap<op_ldb_direct>);
    opcode_handlers[0xF6] = std::addressof(opcodewrap<op_ldb_extended>);
    opcode_handlers[0xC6] = std::addressof(opcodewrap<op_ldb_immediate>);
    opcode_handlers[0xE6] = std::addressof(opcodewrap<op_ldb_indexed>);
    opcode_handlers[0xDC] = std::addressof(opcodewrap<op_ldd_direct>);
    opcode_handlers[0xFC] = std::addressof(opcodewrap<op_ldd_extended>);
    opcode_handlers[0xCC] = std::addressof(opcodewrap<op_ldd_immediate>);
    opcode_handlers[0xEC] = std::addressof(opcodewrap<op_ldd_indexed>);
    opcode_handlers[0xDE] = std::addressof(opcodewrap<op_ldu_direct>);
    opcode_handlers[0xFE] = std::addressof(opcodewrap<op_ldu_extended>);
    opcode_handlers[0xCE] = std::addressof(opcodewrap<op_ldu_immediate>);
    opcode_handlers[0xEE] = std::addressof(opcodewrap<op_ldu_indexed>);
    opcode_handlers[0x9E] = std::addressof(opcodewrap<op_ldx_direct>);
    opcode_handlers[0xBE] = std::addressof(opcodewrap<op_ldx_extended>);
    opcode_handlers[0x8E] = std::addressof(opcodewrap<op_ldx_immediate>);
    opcode_handlers[0xAE] = std::addressof(opcodewrap<op_ldx_indexed>);
    opcode_handlers[0x32] = std::addressof(opcodewrap<op_leas_indexed>);
    opcode_handlers[0x33] = std::addressof(opcodewrap<op_leau_indexed>);
    opcode_handlers[0x30] = std::addressof(opcodewrap<op_leax_indexed>);
    opcode_handlers[0x31] = std::addressof(opcodewrap<op_leay_indexed>);
    opcode_handlers[0x08] = std::addressof(opcodewrap<op_lsl_direct>);
    opcode_handlers[0x78] = std::addressof(opcodewrap<op_lsl_extended>);
    opcode_handlers[0x68] = std::addressof(opcodewrap<op_lsl_indexed>);
    opcode_handlers[0x48] = std::addressof(opcodewrap<op_lsla_inherent>);
    opcode_handlers[0x58] = std::addressof(opcodewrap<op_lslb_inherent>);
    opcode_handlers[0x04] = std::addressof(opcodewrap<op_lsr_direct>);
    opcode_handlers[0x74] = std::addressof(opcodewrap<op_lsr_extended>);
    opcode_handlers[0x64] = std::addressof(opcodewrap<op_lsr_indexed>);
    opcode_handlers[0x44] = std::addressof(opcodewrap<op_lsra_inherent>);
    opcode_handlers[0x54] = std::addressof(opcodewrap<op_lsrb_inherent>);
    opcode_handlers[0x3D] = std::addressof(opcodewrap<op_mul_inherent>);
    opcode_handlers[0x00] = std::addressof(opcodewrap<op_neg_direct>);
    opcode_handlers[0x70] = std::addressof(opcodewrap<op_neg_extended>);
    opcode_handlers[0x60] = std::addressof(opcodewrap<op_neg_indexed>);
    opcode_handlers[0x40] = std::addressof(opcodewrap<op_nega_inherent>);
    opcode_handlers[0x50] = std::addressof(opcodewrap<op_negb_inherent>);
    opcode_handlers[0x12] = std::addressof(opcodewrap<op_nop_inherent>);
    opcode_handlers[0x9A] = std::addressof(opcodewrap<op_ora_direct>);
    opcode_handlers[0xBA] = std::addressof(opcodewrap<op_ora_extended>);
    opcode_handlers[0x8A] = std::addressof(opcodewrap<op_ora_immediate>);
    opcode_handlers[0xAA] = std::addressof(opcodewrap<op_ora_indexed>);
    opcode_handlers[0xDA] = std::addressof(opcodewrap<op_orb_direct>);
    opcode_handlers[0xFA] = std::addressof(opcodewrap<op_orb_extended>);
    opcode_handlers[0xCA] = std::addressof(opcodewrap<op_orb_immediate>);
    opcode_handlers[0xEA] = std::addressof(opcodewrap<op_orb_indexed>);
    opcode_handlers[0x1A] = std::addressof(opcodewrap<op_orcc_immediate>);
    opcode_handlers[0x34] = std::addressof(opcodewrap<op_pshs_immediate>);
    opcode_handlers[0x36] = std::addressof(opcodewrap<op_pshu_immediate>);
    opcode_handlers[0x35] = std::addressof(opcodewrap<op_puls_immediate>);
    opcode_handlers[0x37] = std::addressof(opcodewrap<op_pulu_immediate>);
    opcode_handlers[0x09] = std::addressof(opcodewrap<op_rol_direct>);
    opcode_handlers[0x79] = std::addressof(opcodewrap<op_rol_extended>);
    opcode_handlers[0x69] = std::addressof(opcodewrap<op_rol_indexed>);
    opcode_handlers[0x49] = std::addressof(opcodewrap<op_rola_inherent>);
    opcode_handlers[0x59] = std::addressof(opcodewrap<op_rolb_inherent>);
    opcode_handlers[0x06] = std::addressof(opcodewrap<op_ror_direct>);
    opcode_handlers[0x76] = std::addressof(opcodewrap<op_ror_extended>);
    opcode_handlers[0x66] = std::addressof(opcodewrap<op_ror_indexed>);
    opcode_handlers[0x46] = std::addressof(opcodewrap<op_rora_inherent>);
    opcode_handlers[0x56] = std::addressof(opcodewrap<op_rorb_inherent>);
    opcode_handlers[0x3B] = std::addressof(opcodewrap<op_rti_inherent>);
    opcode_handlers[0x39] = std::addressof(opcodewrap<op_rts_inherent>);
    opcode_handlers[0x92] = std::addressof(opcodewrap<op_sbca_direct>);
    opcode_handlers[0xB2] = std::addressof(opcodewrap<op_sbca_extended>);
    opcode_handlers[0x82] = std::addressof(opcodewrap<op_sbca_immediate>);
    opcode_handlers[0xA2] = std::addressof(opcodewrap<op_sbca_indexed>);
    opcode_handlers[0xD2] = std::addressof(opcodewrap<op_sbcb_direct>);
    opcode_handlers[0xF2] = std::addressof(opcodewrap<op_sbcb_extended>);
    opcode_handlers[0xC2] = std::addressof(opcodewrap<op_sbcb_immediate>);
    opcode_handlers[0xE2] = std::addressof(opcodewrap<op_sbcb_indexed>);
    opcode_handlers[0x1D] = std::addressof(opcodewrap<op_sex_inherent>);
    opcode_handlers[0x97] = std::addressof(opcodewrap<op_sta_direct>);
    opcode_handlers[0xB7] = std::addressof(opcodewrap<op_sta_extended>);
    opcode_handlers[0xA7] = std::addressof(opcodewrap<op_sta_indexed>);
    opcode_handlers[0xD7] = std::addressof(opcodewrap<op_stb_direct>);
    opcode_handlers[0xF7] = std::addressof(opcodewrap<op_stb_extended>);
    opcode_handlers[0xE7] = std::addressof(opcodewrap<op_stb_indexed>);
    opcode_handlers[0xDD] = std::addressof(opcodewrap<op_std_direct>);
    opcode_handlers[0xFD] = std::addressof(opcodewrap<op_std_extended>);
    opcode_handlers[0xED] = std::addressof(opcodewrap<op_std_indexed>);
    opcode_handlers[0xDF] = std::addressof(opcodewrap<op_stu_direct>);
    opcode_handlers[0xFF] = std::addressof(opcodewrap<op_stu_extended>);
    opcode_handlers[0xEF] = std::addressof(opcodewrap<op_stu_indexed>);
    opcode_handlers[0x9F] = std::addressof(opcodewrap<op_stx_direct>);
    opcode_handlers[0xBF] = std::addressof(opcodewrap<op_stx_extended>);
    opcode_handlers[0xAF] = std::addressof(opcodewrap<op_stx_indexed>);
    opcode_handlers[0x90] = std::addressof(opcodewrap<op_suba_direct>);
    opcode_handlers[0xB0] = std::addressof(opcodewrap<op_suba_extended>);
    opcode_handlers[0x80] = std::addressof(opcodewrap<op_suba_immediate>);
    opcode_handlers[0xA0] = std::addressof(opcodewrap<op_suba_indexed>);
    opcode_handlers[0xD0] = std::addressof(opcodewrap<op_subb_direct>);
    opcode_handlers[0xF0] = std::addressof(opcodewrap<op_subb_extended>);
    opcode_handlers[0xC0] = std::addressof(opcodewrap<op_subb_immediate>);
    opcode_handlers[0xE0] = std::addressof(opcodewrap<op_subb_indexed>);
    opcode_handlers[0x93] = std::addressof(opcodewrap<op_subd_direct>);
    opcode_handlers[0xB3] = std::addressof(opcodewrap<op_subd_extended>);
    opcode_handlers[0x83] = std::addressof(opcodewrap<op_subd_immediate>);
    opcode_handlers[0xA3] = std::addressof(opcodewrap<op_subd_indexed>);
    opcode_handlers[0x3F] = std::addressof(opcodewrap<op_swi1_inherent>);
    opcode_handlers[0x13] = std::addressof(opcodewrap<op_sync_inherent>);
    opcode_handlers[0x1F] = std::addressof(opcodewrap<op_tfr_immediate>);
    opcode_handlers[0x0D] = std::addressof(opcodewrap<op_tst_direct>);
    opcode_handlers[0x7D] = std::addressof(opcodewrap<op_tst_extended>);
    opcode_handlers[0x6D] = std::addressof(opcodewrap<op_tst_indexed>);
    opcode_handlers[0x4D] = std::addressof(opcodewrap<op_tsta_inherent>);
    opcode_handlers[0x5D] = std::addressof(opcodewrap<op_tstb_inherent>);

    opcode_handlers_page1[0x3f] = std::addressof(opcodewrap<op_swi2_inherent>);
    opcode_handlers_page2[0x3f] = std::addressof(opcodewrap<op_swi3_inherent>);
    opcode_handlers_page1[0x9f] = std::addressof(opcodewrap<op_sty_direct>);
    opcode_handlers_page1[0xaf] = std::addressof(opcodewrap<op_sty_indexed>);
    opcode_handlers_page1[0xbf] = std::addressof(opcodewrap<op_sty_extended>);
    opcode_handlers_page1[0xdf] = std::addressof(opcodewrap<op_sts_direct>);
    opcode_handlers_page1[0xef] = std::addressof(opcodewrap<op_sts_indexed>);
    opcode_handlers_page1[0xff] = std::addressof(opcodewrap<op_sts_extended>);
    opcode_handlers_page1[0x83] = std::addressof(opcodewrap<op_cmpd_immediate>);
    opcode_handlers_page1[0x8c] = std::addressof(opcodewrap<op_cmpy_immediate>);
    opcode_handlers_page1[0x8e] = std::addressof(opcodewrap<op_ldy_immediate>);
    opcode_handlers_page1[0x93] = std::addressof(opcodewrap<op_cmpd_direct>);
    opcode_handlers_page1[0x9c] = std::addressof(opcodewrap<op_cmpy_direct>);
    opcode_handlers_page1[0x9e] = std::addressof(opcodewrap<op_ldy_direct>);
    opcode_handlers_page1[0xa3] = std::addressof(opcodewrap<op_cmpd_indexed>);
    opcode_handlers_page1[0xac] = std::addressof(opcodewrap<op_cmpy_indexed>);
    opcode_handlers_page1[0xae] = std::addressof(opcodewrap<op_ldy_indexed>);
    opcode_handlers_page1[0xb3] = std::addressof(opcodewrap<op_cmpd_extended>);
    opcode_handlers_page1[0xbc] = std::addressof(opcodewrap<op_cmpy_extended>);
    opcode_handlers_page1[0xbe] = std::addressof(opcodewrap<op_ldy_extended>);
    opcode_handlers_page1[0xce] = std::addressof(opcodewrap<op_lds_immediate>);
    opcode_handlers_page1[0xde] = std::addressof(opcodewrap<op_lds_direct>);
    opcode_handlers_page1[0xee] = std::addressof(opcodewrap<op_lds_indexed>);
    opcode_handlers_page1[0xfe] = std::addressof(opcodewrap<op_lds_extended>);
    opcode_handlers_page2[0x83] = std::addressof(opcodewrap<op_cmpu_immediate>);
    opcode_handlers_page2[0x8c] = std::addressof(opcodewrap<op_cmps_immediate>);
    opcode_handlers_page2[0x93] = std::addressof(opcodewrap<op_cmpu_direct>);
    opcode_handlers_page2[0x9c] = std::addressof(opcodewrap<op_cmps_direct>);
    opcode_handlers_page2[0xa3] = std::addressof(opcodewrap<op_cmpu_indexed>);
    opcode_handlers_page2[0xac] = std::addressof(opcodewrap<op_cmps_indexed>);
    opcode_handlers_page2[0xb3] = std::addressof(opcodewrap<op_cmpu_extended>);
    opcode_handlers_page2[0xbc] = std::addressof(opcodewrap<op_cmps_extended>);

    // branches
    opcode_handlers[0x20] = std::addressof(opcodewrap<op_bra_inherent>);
    opcode_handlers[0x16] = std::addressof(opcodewrap<op_lbra_inherent>);
    opcode_handlers[0x21] = std::addressof(opcodewrap<op_brn_inherent>);
    opcode_handlers_page1[0x21] = std::addressof(opcodewrap<op_lbrn_inherent>);
    opcode_handlers[0x25] = std::addressof(opcodewrap<op_bcs_inherent>);
    opcode_handlers_page1[0x25] = std::addressof(opcodewrap<op_lbcs_inherent>);
    opcode_handlers[0x24] = std::addressof(opcodewrap<op_bcc_inherent>);
    opcode_handlers_page1[0x24] = std::addressof(opcodewrap<op_lbcc_inherent>);
    opcode_handlers[0x22] = std::addressof(opcodewrap<op_bhi_inherent>);
    opcode_handlers_page1[0x22] = std::addressof(opcodewrap<op_lbhi_inherent>);
    opcode_handlers[0x23] = std::addressof(opcodewrap<op_bls_inherent>);
    opcode_handlers_page1[0x23] = std::addressof(opcodewrap<op_lbls_inherent>);
    opcode_handlers[0x27] = std::addressof(opcodewrap<op_beq_inherent>);
    opcode_handlers_page1[0x27] = std::addressof(opcodewrap<op_lbeq_inherent>);
    opcode_handlers[0x26] = std::addressof(opcodewrap<op_bne_inherent>);
    opcode_handlers_page1[0x26] = std::addressof(opcodewrap<op_lbne_inherent>);
    opcode_handlers[0x2E] = std::addressof(opcodewrap<op_bgt_inherent>);
    opcode_handlers_page1[0x2E] = std::addressof(opcodewrap<op_lbgt_inherent>);
    opcode_handlers[0x2D] = std::addressof(opcodewrap<op_blt_inherent>);
    opcode_handlers_page1[0x2D] = std::addressof(opcodewrap<op_lblt_inherent>);
    opcode_handlers[0x2C] = std::addressof(opcodewrap<op_bge_inherent>);
    opcode_handlers_page1[0x2C] = std::addressof(opcodewrap<op_lbge_inherent>);
    opcode_handlers[0x2F] = std::addressof(opcodewrap<op_ble_inherent>);
    opcode_handlers_page1[0x2F] = std::addressof(opcodewrap<op_lble_inherent>);
    opcode_handlers[0x2A] = std::addressof(opcodewrap<op_bpl_inherent>);
    opcode_handlers_page1[0x2A] = std::addressof(opcodewrap<op_lbpl_inherent>);
    opcode_handlers[0x2B] = std::addressof(opcodewrap<op_bmi_inherent>);
    opcode_handlers_page1[0x2B] = std::addressof(opcodewrap<op_lbmi_inherent>);
    opcode_handlers[0x29] = std::addressof(opcodewrap<op_bvs_inherent>);
    opcode_handlers_page1[0x29] = std::addressof(opcodewrap<op_lbvs_inherent>);
    opcode_handlers[0x28] = std::addressof(opcodewrap<op_bvc_inherent>);
    opcode_handlers_page1[0x28] = std::addressof(opcodewrap<op_lbvc_inherent>);
    opcode_handlers[0x8D] = std::addressof(opcodewrap<op_bsr_relative>);
    opcode_handlers[0x17] = std::addressof(opcodewrap<op_lbsr_relative>);

}

m6809_error_t M6809::Execute(int &cycles, m6809_interrupt_t irq)
{
    // if there is an interrupt and the SYNC had been called, then end the sync wait
    if (irq != NONE && irq_state == IRQ_SYNC) {
        irq_state = IRQ_NORMAL;
    }

    // IRQ and NMI are the same, except NMI cannot be masked
    if ((irq == IRQ && !registers.flags.I) || irq == NMI)
    {
        if (irq_state == IRQ_NORMAL) {
            registers.flags.E = 1;
            // Push PC,CC
            op_push<reg_sp, reg_usp>()(*this, 0xff, cycles);
        }
        registers.CC |= FLAG_I|FLAG_F;

        registers.PC = Read16((irq == IRQ) ? IRQ_VECTOR : NMI_VECTOR);
        irq_state = IRQ_NORMAL;
    }
    // handled if the FIRQ interrupt isn't masked
    else if (irq == FIRQ && !registers.flags.F)
    {
        if (irq_state == IRQ_NORMAL) {
            registers.flags.E = 0;
            // Push PC,CC
            op_push<reg_sp, reg_usp>()(*this, 0x81, cycles);
        }
        registers.CC |= FLAG_I|FLAG_F;

        registers.PC = Read16(FIRQ_VECTOR);
        irq_state = IRQ_NORMAL;
    }


    // if the IRQ state is WAIT or SYNC, then just clock one cycle
    if (irq_state != IRQ_NORMAL)
    {
        return E_SUCCESS;
    }

    uint8_t opcode = NextOpcode();

    if (opcode == 0x10)
    {
        opcode = NextOpcode();
        opcode_handler_t opcode_handler = opcode_handlers_page1[opcode];
        if (opcode_handler) {
            opcode_handler(*this, cycles);
            return E_SUCCESS;
        }
        else
        {
            return E_UNKNOWN_OPCODE_PAGE1;
        }
    }
    else if (opcode == 0x11)
    {
        opcode = NextOpcode();
        opcode_handler_t opcode_handler = opcode_handlers_page2[opcode];
        if (opcode_handler) {
            opcode_handler(*this, cycles);
            return E_SUCCESS;
        }
        else
        {
            return E_UNKNOWN_OPCODE_PAGE2;
        }
    }
    else
    {
        opcode_handler_t opcode_handler = opcode_handlers[opcode];
        if (opcode_handler) {
            opcode_handler(*this, cycles);
            return E_SUCCESS;
        }
        else {
            return E_UNKNOWN_OPCODE;
        }
    }
}

void M6809::SetReadCallback(M6809::read_callback_t func, intptr_t ref)
{
    read_callback_func = func;
    read_callback_ref = ref;

    dis_.SetReadCallback(func, ref);
}

void M6809::SetWriteCallback(M6809::write_callback_t func, intptr_t ref)
{
    write_callback_func = func;
    write_callback_ref = ref;
}

void M6809::Reset()
{
    registers.A = 0;
    registers.B = 0;
    registers.X = 0;
    registers.Y = 0;
    registers.USP = 0;
    registers.SP = 0;
    registers.DP = 0;
    registers.CC = FLAG_I | FLAG_F;

    // reset sets the PC to the reset vector found at $FFFE
    registers.PC = Read16(RESET_VECTOR);
    //printf("Reset Vector: $%04x\n", registers.PC);
}

