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
    if (size <= rom_.size()) {
        memcpy(rom_.data(), data, size);
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

uint8_t Cartridge::Read(uint16_t addr)
{
    return rom_[addr];
}

void Cartridge::Write(uint16_t addr, uint8_t data)
{
    (void)addr;
    (void)data;
}
