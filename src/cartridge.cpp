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
#include "cartridge.h"
#include <cstring>

void Cartridge::Load(const uint8_t *data, size_t size)
{
    if (size <= MAX_ROM_SIZE) {
        // if the ROM is larger than 32K and smaller than or equal to 64K then it uses PB7 for bank switching
        memcpy(rom_.data(), data, size);

        if (size <= REGULAR_ROM_SIZE)
        {
            memcpy(rom_.data()+REGULAR_ROM_SIZE, data, size);
        }
        else
            printf("[CART]: Loading a bank switched ROM\n");
        is_loaded_flag_ = true;
    }
    else
    {
        Unload();
    }

}

void Cartridge::Unload()
{
    rom_.fill(0);
    is_loaded_flag_ = false;
}

bool Cartridge::is_loaded()
{
    return is_loaded_flag_;
}

uint8_t Cartridge::Read(uint16_t addr, uint8_t pb6)
{
    // bank switch with pb6 by setting the MSB to pb6
    return rom_[(addr & ~0x8000) | ((pb6 ^ 1) << 15)];
}

void Cartridge::Write(uint16_t addr, uint8_t data, uint8_t pb6)
{
    (void)addr;
    (void)data;
    (void)pb6;
}
