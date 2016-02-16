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
#include <cstring>
#include <cstdio>
#include <cmath>
#include <array>
#include "ay38910.h"

void AY38910::Step(uint8_t bus, uint8_t bc1, uint8_t bc2, uint8_t bdir)
{
    switch(bdir << 2 | bc2 << 1 | bc1)
    {
        case PSG_NACT: // disabled
        case PSG_IAB:
        case PSG_DW:
            break;
        case PSG_LATCH_ADDR: // latch address
        case PSG_LATCH_BAR:
        case PSG_LATCH_INTAK:
            // NOTE: address are octal in the documentation
            addr = (uint8_t)(bus & 0xf);
            break;
        case PSG_DWS:
            Write(addr, bus);
            break;
        case PSG_DTS:
            // read callback
            if (store_reg_func) {
                if (addr == PSG_REG_PORTA) { // read from IO
                    if (read_io_func)
                        store_reg_func(store_reg_ref, read_io_func(read_io_ref));
                }
                else { // read from regular register
                    store_reg_func(store_reg_ref, regs[addr]);
                }
            }
            break;
        default:
            break;
    }
}

void AY38910::Write(uint8_t reg, uint8_t value)
{
    regs[reg] = value;

    switch(reg)
    {
        case PSG_REG_A_FINE:
        case PSG_REG_A_COARSE:
            channel_a.set_period(regs[PSG_REG_A_COARSE], regs[PSG_REG_A_FINE]);
            break;

        case PSG_REG_B_FINE: // same as period A
        case PSG_REG_B_COARSE:
            channel_b.set_period(regs[PSG_REG_B_COARSE], regs[PSG_REG_B_FINE]);
            break;

        case PSG_REG_C_FINE: // same as period A/B
        case PSG_REG_C_COARSE:
            channel_c.set_period(regs[PSG_REG_C_COARSE], regs[PSG_REG_C_FINE]);
            break;

        case PSG_REG_NOISE:
            break;

        case PSG_REG_MIXER_CTRL:
            // disable and enable the channels and io
            break;
        case PSG_REG_A_AMPL:
            channel_a.amplitude_mode = (uint8_t) (value & 0x10);
            channel_a.amplitude_fixed = amplitude_table[value & 0xf];
            break;
        case PSG_REG_B_AMPL:
            channel_b.amplitude_mode = (uint8_t) (value & 0x10);
            channel_b.amplitude_fixed = amplitude_table[value & 0xf];
            break;
        case PSG_REG_C_AMPL:
            channel_c.amplitude_mode = (uint8_t) (value & 0x10);
            channel_c.amplitude_fixed = amplitude_table[value & 0xf];
            break;
        case PSG_REG_ENV_FINE:
        case PSG_REG_ENV_COARSE:
            break;
        case PSG_REG_ENV_CTRL:
            // control the shape of the envelope
            break;

        default:break;
    }
}

void AY38910::SetIOReadCallback(AY38910::read_io_callback func, intptr_t ref)
{
    read_io_func = func;
    read_io_ref = ref;
}

void AY38910::SetRegStoreCallback(AY38910::store_reg_callback func, intptr_t ref)
{
    store_reg_func = func;
    store_reg_ref = ref;
}

void AY38910::FillBuffer(uint8_t *buffer, size_t length)
{
    memset(buffer, 0, length);

    for (int i = 0; i < length; i++)
    {
        int16_t ampl_a = (int16_t) ((regs[PSG_REG_MIXER_CTRL] & 0x08) ? channel_a.step() : 0);
        int16_t ampl_b = (int16_t) ((regs[PSG_REG_MIXER_CTRL] & 0x10) ? channel_b.step() : 0);
        int16_t ampl_c = (int16_t) ((regs[PSG_REG_MIXER_CTRL] & 0x20) ? channel_c.step() : 0);
        *(buffer++) = (int8_t)((int16_t)((ampl_a + ampl_b + ampl_c) / 3.0) >> 8);
    }
}
