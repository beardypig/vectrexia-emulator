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
#ifndef VECTREXIA_VECTREXIA_H
#define VECTREXIA_VECTREXIA_H

#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <array>
#include <vector>
#include <memory>

#if _MSC_VER >= 1910 && !__INTEL_COMPILER
#include "win32.h"
#endif

#include "cartridge.h"
#include "sysrom.h"
#include "m6809.h"
#include "via6522.h"
#include "ay38910.h"
#include "vectorizer.h"

class Vectrex
{
    const char *kName_ = "Vectrexia";
    const char *kVersion_ = "0.2.0";

    // memory areas
    // 8K of system ROM
    // 1K of system RAM
    const std::array<uint8_t, 8192> sysrom_ = system_bios;
    std::array<uint8_t, 1024> ram_{};

    // This structure represents the values of the potentiometers and the buttons a vectrex controller
    struct
    {
        uint8_t pot_x, pot_y;
        uint8_t btn_1, btn_2, btn_3, btn_4;
    } p1_joystick, p2_joystick;
    uint8_t joystick_compare;
    uint8_t psg_port;

public:
    std::unique_ptr<Cartridge> cartridge_{};
    std::unique_ptr<M6809> cpu_{};
    std::unique_ptr<VIA6522> via_{};
    std::unique_ptr<AY38910> psg_{};
    Vectorizer vector_buffer_;
    uint64_t cycles;

    Vectrex() noexcept;
    Vectrex(const Vectrex&) = delete;
    Vectrex(Vectrex&&) = delete;
    Vectrex &operator=(const Vectrex&) = delete;
    Vectrex &operator=(Vectrex&&) = delete;
    ~Vectrex() = default;

    void Reset();
    uint64_t Run(uint64_t cycles);

    bool LoadCartridge(const uint8_t *data, size_t size);
    void UnloadCartridge();

    const char *GetName();
    const char *GetVersion();

    uint8_t Read(uint16_t addr);
    void Write(uint16_t addr, uint8_t data);

    void message(const char *fmt, ...);

    VectorBuffer *getFramebuffer();
    DebugBuffer *getDebugbuffer();

    uint8_t ReadPortA();
    uint8_t ReadPortB();
    void UpdateJoystick(uint8_t porta, uint8_t portb);
    void SetPlayerOne(uint8_t x, uint8_t y, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
    void SetPlayerTwo(uint8_t x, uint8_t y, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
    uint8_t ReadPSGIO();
    void StorePSGReg(uint8_t reg);
    M6809 &GetM6809();
};

#endif //VECTREXIA_VECTREXIA_H
