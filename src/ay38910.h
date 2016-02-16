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
#ifndef VECTREXIA_AY38910_H
#define VECTREXIA_AY38910_H

#include <stdint.h>
#include <algorithm>

const double pi = std::acos(-1);

enum {
    PSG_NACT,
    PSG_LATCH_ADDR,
    PSG_IAB,
    PSG_DTS, // read
    PSG_LATCH_BAR,
    PSG_DW,
    PSG_DWS, // write
    PSG_LATCH_INTAK
};

enum {
    PSG_REG_A_FINE     = 000,
    PSG_REG_A_COARSE   = 001,
    PSG_REG_B_FINE     = 002,
    PSG_REG_B_COARSE   = 003,
    PSG_REG_C_FINE     = 004,
    PSG_REG_C_COARSE   = 005,
    PSG_REG_NOISE      = 006,
    PSG_REG_MIXER_CTRL = 007,
    PSG_REG_A_AMPL     = 010,
    PSG_REG_B_AMPL     = 011,
    PSG_REG_C_AMPL     = 012,
    PSG_REG_ENV_FINE   = 013,
    PSG_REG_ENV_COARSE = 014,
    PSG_REG_ENV_CTRL   = 015,
    PSG_REG_PORTA      = 016,
    PSG_REG_PORTB      = 017
};

class AY38910
{
    using read_io_callback = uint8_t (*)(intptr_t);
    using store_reg_callback = void (*)(intptr_t, uint8_t);

    template <int sample_rate>
    struct channel_t
    {
        uint16_t period          = 0; // 12-bit period for the channel
        uint8_t  amplitude_mode  = 0; // fixed or envelope variable
        int16_t  amplitude_fixed;
        double   velocity_, radian_ = 0.0;

        void set_period(uint8_t coarse, uint8_t fine)
        {
            double frequency;
            // from the datasheet (256CT10 + FT10)
            // the tone period is made up of the lower 4 bits of the coarse tone register and the fine register
            // the maximum value for period is 4095 and the minimum value is 1
            period = std::max<uint16_t>((uint16_t) (((coarse & 0xf) << 8) | fine), 1);
            frequency = 1.5e6/(period * 16);
            // the angular velocity for the sine wave
            velocity_ = (2 * pi * frequency) / sample_rate;
        }

        int16_t step()
        {
            double out = radian_;
            radian_ += velocity_;
            return (int16_t) ((std::sin(radian_) > 0.0 ? 1.0 : -1.0) * amplitude());
        }

        int16_t amplitude() const
        {
            if (!amplitude_mode)
                return amplitude_fixed;
            else
                return 0;
        }
    };

    uint8_t regs[0xf];
    uint8_t addr;
    channel_t<44100> channel_a, channel_b, channel_c;

    const int16_t amplitude_table[16] = { 0x0000, 0x0055, 0x0079, 0x00AB, 0x00F1, 0x0155, 0x01E3, 0x02AA,
                                          0x03C5, 0x0555, 0x078B, 0x0AAB, 0x0F16, 0x1555, 0x1E2B, 0x2AAA };

    // port a/b read callbacks
    store_reg_callback store_reg_func = nullptr;
    intptr_t           store_reg_ref = 0;
    read_io_callback   read_io_func = nullptr;
    intptr_t           read_io_ref = 0;

public:
    void Step(uint8_t bus, uint8_t bc1, uint8_t bc2, uint8_t bdir);
    void SetIOReadCallback(read_io_callback func, intptr_t ref);
    void SetRegStoreCallback(store_reg_callback func, intptr_t ref);
    void Write(uint8_t reg, uint8_t value);
    void FillBuffer(uint8_t * const buffer, size_t length);
};


#endif //VECTREXIA_AY38910_H
