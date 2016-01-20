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
#include <array>
#include <vector>
#include <memory>
#include "cartridge.h"
#include "sysrom.h"


class Vectrex
{
    const char *kName_ = "Vectrexia";
    const char *kVersion_ = "0.1.0";

    // memory areas
    // 8K of system ROM
    // 1K of system RAM
    const std::array<uint8_t, 8192> sysrom_ = system_bios;
    std::array<uint8_t, 1024> ram_;

    std::unique_ptr<Cartridge> cartridge_;


public:
    Vectrex() = default;
    void Reset();
    long Run(long cycles);

    bool LoadCartridge(const uint8_t *data, size_t size);
    void UnloadCartridge();

    const char *GetName();
    const char *GetVersion();

};

#endif //VECTREXIA_VECTREXIA_H
