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
        // from the datasheet (256CT10 + FT10)
        // the tone period is made up of the lower 4 bits of the coarse tone register and the fine register
        // the maximum value for period is 4095 and the minimum value is 1
        case PSG_REG_A_FINE:
        case PSG_REG_A_COARSE:
            channel_a.setPeriod((uint8_t) (regs[PSG_REG_A_COARSE] & 0xf), regs[PSG_REG_A_FINE]);
            break;

        case PSG_REG_B_FINE: // same as period A
        case PSG_REG_B_COARSE:
            channel_b.setPeriod((uint8_t) (regs[PSG_REG_B_COARSE] & 0xf), regs[PSG_REG_B_FINE]);
            break;

        case PSG_REG_C_FINE: // same as period A/B
        case PSG_REG_C_COARSE:
            channel_c.setPeriod((uint8_t) (regs[PSG_REG_C_COARSE] & 0xf), regs[PSG_REG_C_FINE]);
            break;

        case PSG_REG_NOISE:
            channel_noise.setPeriod((uint8_t) (regs[PSG_REG_NOISE] & 0x1f));
            break;

        case PSG_REG_MIXER_CTRL:
            // disable and enable the channels and io
            channel_a.enabled = !(value & 1);
            channel_a.noise_enabled = !(value & (1 << 3));
            channel_b.enabled = !(value & 2);
            channel_b.noise_enabled = !(value & (2 << 3));
            channel_c.enabled = !(value & 4);
            channel_c.noise_enabled = !(value & (4 << 3));
            break;

        case PSG_REG_A_AMPL:
            channel_a.amplitude_mode = (uint8_t)((value >> 4) & 1);
            channel_a.amplitude_fixed = (uint8_t) (value & 0xf);
            break;
        case PSG_REG_B_AMPL:
            channel_b.amplitude_mode = (uint8_t)((value >> 4) & 1);
            channel_b.amplitude_fixed = (uint8_t) (value & 0xf);
            break;
        case PSG_REG_C_AMPL:
            channel_c.amplitude_mode = (uint8_t)((value >> 4) & 1);
            channel_c.amplitude_fixed = (uint8_t) (value & 0xf);
            break;

        case PSG_REG_ENV_FINE:
        case PSG_REG_ENV_COARSE:
            envelope.setPeriod(regs[PSG_REG_ENV_COARSE], regs[PSG_REG_ENV_FINE]);
            break;
        case PSG_REG_ENV_CTRL:
            // control the shape of the envelope
            envelope.setControl((uint8_t) (value & 0xf));
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
    int16_t ampl_a = 0, ampl_b = 0, ampl_c = 0;
    memset(buffer, 0, length);

    for (int i = 0; i < length; i++)
    {
        auto noise = channel_noise.step();
        auto envelope_counter = envelope.step();

        ampl_a = channel_a.step(noise, envelope_counter);
        ampl_b = channel_b.step(noise, envelope_counter);
        ampl_c = channel_c.step(noise, envelope_counter);

        *(buffer++) = (uint8_t)((uint16_t)(((channel_a_on ? ampl_a : 0) +
                                            (channel_b_on ? ampl_b : 0) +
                                            (channel_c_on ? ampl_c : 0)) / 3.0) >> 8);
    }
}
