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
#ifndef VECTREXIA_CARTRIDGE_H
#define VECTREXIA_CARTRIDGE_H

#include <stdint.h>
#include <array>

class Cartridge
{
    const int REGULAR_ROM_SIZE = 32768;
    const int MAX_ROM_SIZE = 65536;
    // 64K of cartridge for bank switched ROMs
    std::array<uint8_t, 65536> rom_ = {};
    bool is_loaded_flag_ = false;

public:
    Cartridge() = default;

    void Load(const uint8_t* data, size_t size);
    void Unload();
    bool is_loaded();

    uint8_t Read(uint16_t addr, uint8_t pb6=1);
    void Write(uint16_t addr, uint8_t data, uint8_t pb6=1);
};


#endif //VECTREXIA_CARTRIDGE_H
