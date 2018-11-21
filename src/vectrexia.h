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
#include "joystick.h"
#include "sw_vectorizer.h"

class Vectrex
{
    const char *kName_ = "Vectrexia";
    const char *kVersion_ = "0.2.0";

    // memory areas
    // 8K of system ROM
    // 1K of system RAM
    const std::array<uint8_t, 8192> sysrom_ = system_bios;
    std::array<uint8_t, 1024> ram_{};

    uint8_t joystick_compare;
    uint8_t psg_port;

public:
    std::unique_ptr<Cartridge> cartridge{};
    std::unique_ptr<Joystick> joy{};
    std::unique_ptr<M6809> cpu{};
    std::unique_ptr<VIA6522> via{};
    std::unique_ptr<AY38910> psg{};
    std::unique_ptr<CD4052B> mpx{};
    std::unique_ptr<MC1408P8> dac{};
    std::unique_ptr<SWVectorizer> vec{};
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

    framebuffer_t *getFramebuffer();

    void SetPlayerOne(uint8_t x, uint8_t y, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
    void SetPlayerTwo(uint8_t x, uint8_t y, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
    M6809 &GetM6809();
};

#endif //VECTREXIA_VECTREXIA_H
