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
#ifndef VECTREXIA_VECTREXIA_H
#define VECTREXIA_VECTREXIA_H

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <array>
#include <vector>
#include <memory>
#include "cartridge.h"
#include "sysrom.h"
#include "m6809.h"
#include "via6522.h"
#include "vectorizer.h"
#include "ay38910.h"


class Vectrex
{
    const char *kName_ = "Vectrexia";
    const char *kVersion_ = "0.1.0";

    // memory areas
    // 8K of system ROM
    // 1K of system RAM
    const std::array<uint8_t, 8192> sysrom_ = system_bios;
    std::array<uint8_t, 1024> ram_;

    struct {
        unsigned char pot0, pot1,
                pot2, pot3;
        unsigned char sw0, sw1, sw2, sw3,
                sw4, sw5, sw6, sw7;
    } joysticks;

    std::unique_ptr<Cartridge> cartridge_;
    std::unique_ptr<M6809> cpu_;
    std::unique_ptr<VIA6522> via_;
    std::unique_ptr<AY38910> psg_;
    Vectorizer vector_buffer;

    uint64_t cycles;
    uint8_t joystick_compare = 0x7f;
    uint8_t psg_port = 0xff; // buttons are connected the IO port on the PSG

public:
    Vectrex();
    void Reset();
    uint64_t Run(long cycles);

    bool LoadCartridge(const uint8_t *data, size_t size);
    void UnloadCartridge();

    const char *GetName();
    const char *GetVersion();

    uint8_t Read(uint16_t addr);
    void Write(uint16_t addr, uint8_t data);

    void PeripheralStep(uint8_t porta, uint8_t portb, uint8_t ca1, uint8_t ca2, uint8_t cb1, uint8_t cb2);

    void message(const char *fmt, ...);

    M6809 &GetM6809();
    VIA6522 &GetVIA6522();
    Vectorizer &GetVectorizer();

    std::array<float, 135300> getFramebuffer();

    uint8_t ReadPortB();
    uint8_t ReadPortA();
    void SetPlayerOne(uint8_t x, uint8_t y, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
    void SetPlayerTwo(uint8_t x, uint8_t y, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
    void UpdateJoystick(uint8_t porta, uint8_t portb);
    uint8_t ReadPSGIO();
    void StorePSGReg(uint8_t reg);
};

#endif //VECTREXIA_VECTREXIA_H
