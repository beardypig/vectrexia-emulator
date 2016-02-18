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
    struct periodic_t
    {
        uint16_t period_ = 1;
        double frequency_;
        double velocity_ = 0.0;

        const int16_t amplitude_table[16] = { 0x0000, 0x0055, 0x0079, 0x00AB, 0x00F1, 0x0155, 0x01E3, 0x02AA,
                                              0x03C5, 0x0555, 0x078B, 0x0AAB, 0x0F16, 0x1555, 0x1E2B, 0x2AAA };

        double setPeriod(uint8_t coarse, uint8_t fine)
        {
            period_ = std::max<uint16_t>((uint16_t) ((coarse << 8) | fine), 1);
            frequency_ = 1.5e6/(period_ * 16);
            velocity_ = frequency_ / (double)sample_rate;
            return frequency_;
        }
        double setPeriod(uint8_t fine)
        {
            return setPeriod(0, fine);
        }

    };

    template <int sample_rate>
    struct channel_t : periodic_t<sample_rate>
    {
        uint8_t  amplitude_mode  = 0; // fixed or envelope variable
        uint8_t  amplitude_fixed;
        bool enabled, noise_enabled;
        double radian_;

        int16_t step(int16_t noise, uint8_t envelope)
        {
            int16_t out;

            if (noise_enabled)
            {
                out = noise;
            }
            else if (!enabled) // redundant - && !noise_enabled)
            {
                // if the channel is disable and the noise is disabled then the sound can be modulated by the volume
                out = 1;
            }
            else // enabled channel
            {
                out = (int16_t) ((std::sin(radian_) > 0.0 ? 1.0 : -1.0));
            }

            radian_ += periodic_t<sample_rate>::velocity_;
            return out * amplitude(envelope);

        }

        inline int16_t amplitude(uint8_t envelope_amplitude) const
        {
            if (!amplitude_mode)
                return periodic_t<sample_rate>::amplitude_table[amplitude_fixed];
            else
                return periodic_t<sample_rate>::amplitude_table[envelope_amplitude];
        }
    };

    template <int sample_rate>
    struct noise_t : periodic_t<sample_rate>
    {
        uint32_t rng = 1;
        double tick_count_ = 0.0;
        // returns one sample, the time period covered by this method is 1/sample_rate
        // the rng is ticking a long at frequency = 1.5e6/(period * 16);
        int16_t step()
        {
            while (tick_count_ > 1.0)
            {
                rng ^= (((rng & 1) ^ ((rng >> 3) & 1)) << 17);
                rng >>= 1;
                tick_count_ -= 1.0;
            }

            tick_count_ += periodic_t<sample_rate>::velocity_;
            return (int16_t) (rng & 1);
        }
    };

    template <int sample_rate>
    struct envelope_t : periodic_t<sample_rate>
    {
        uint8_t counter = 0, envelope_cycle = 0;
        uint8_t hold, attack, alternate, cont, direction;
        bool holding = false;
        double tick_count_ = 0.0;

        void setControl(uint8_t value)
        {
            hold      = (uint8_t) (value & 1);
            attack    = (uint8_t) ((value >> 1) & 1);
            alternate = (uint8_t) ((value >> 2) & 1);
            cont      = (uint8_t) ((value >> 3) & 1);
            direction = attack;
            holding   = false;
            //printf("Envelope control changed: continue=%d, attack=%d, alternate=%d, hold=%d\n",
            //           cont, attack, alternate, hold);
        }

        inline void step_cycle()
        {
            // the divider is really 256 but we need the counter to go at 16 * that
            // ie. a cycle last 16 ticks (16*16 = 265)
            if ((envelope_cycle++ & 0xf) == 0xf)
            {
                envelope_cycle = 0;

                holding = hold;
                // toggle the direction of the counter
                if (alternate)
                {
                    direction ^= 1;
                }

                if (cont)
                {
                    counter = 0;
                }
            }
        }

        uint8_t step()
        {
            while (tick_count_ > 1.0)
            {
                //if ((!direction && counter > 0) || (direction && counter < 0xf))
                //    printf("ticking envelop sub-cycle: counter=%d going %s (holding: %d)\n", counter, (direction) ? "up" : "down", holding);

                step_cycle();
                if (!holding)
                {
                    // direction can be toggled by the alternate flag
                    // count down to 0 when direction/attack is 0
                    if (!direction && counter > 0)
                    {
                        counter -= 1;
                    }
                    // count up to 0xf when direction/attack is 1
                    else if (direction && counter < 0xf)
                    {
                        counter += 1;
                    }
                }

                tick_count_ -= 1.0;
            }

            tick_count_ += periodic_t<sample_rate>::velocity_;
            return (uint8_t) (counter & 0xf);
        }
    };

    uint8_t regs[0xf];
    uint8_t addr;


    // port a/b read callbacks
    store_reg_callback store_reg_func = nullptr;

    intptr_t           store_reg_ref = 0;
    read_io_callback   read_io_func = nullptr;
    intptr_t           read_io_ref = 0;
public:

    bool channel_a_on = true, channel_b_on = true, channel_c_on = true;

    void Step(uint8_t bus, uint8_t bc1, uint8_t bc2, uint8_t bdir);
    void SetIOReadCallback(read_io_callback func, intptr_t ref);
    void SetRegStoreCallback(store_reg_callback func, intptr_t ref);
    void Write(uint8_t reg, uint8_t value);
    void FillBuffer(uint8_t * const buffer, size_t length);

    channel_t<44100> channel_a, channel_b, channel_c;
    noise_t<44100> channel_noise;
    envelope_t<44100> envelope;
};


#endif //VECTREXIA_AY38910_H
